// SPDX-License-Identifier: GPL-2.0+
/*
 *  stale_handle.c - attempt to create a stale handle and open it
 *  Copyright (C) 2010 Red Hat, Inc. All Rights reserved.
 */

#define TEST_UTIME

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <xfs/xfs.h>
#include <xfs/handle.h>

#define NUMFILES 1024
int main(int argc, char **argv)
{
	int	i;
	int	fd;
	int	ret;
	int	failed = 0;
	char	fname[MAXPATHLEN];
	char	*test_dir;
	void	*handle[NUMFILES];
	size_t	hlen[NUMFILES];
	char	fshandle[256];
	size_t	fshlen;
	struct stat st;


	if (argc != 2) {
		fprintf(stderr, "usage: stale_handle test_dir\n");
		return EXIT_FAILURE;
	}

	test_dir = argv[1];
	if (stat(test_dir, &st) != 0) {
		perror("stat");
		return EXIT_FAILURE;
	}

	ret = path_to_fshandle(test_dir, (void **)fshandle, &fshlen);
	if (ret < 0) {
		perror("path_to_fshandle");
		return EXIT_FAILURE;
	}

	/*
	 * create a large number of files to force allocation of new inode
	 * chunks on disk.
	 */
	for (i=0; i < NUMFILES; i++) {
		sprintf(fname, "%s/file%06d", test_dir, i);
		fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) {
			printf("Warning (%s,%d), open(%s) failed.\n", __FILE__, __LINE__, fname);
			perror(fname);
			return EXIT_FAILURE;
		}
		close(fd);
	}

	/* sync to get the new inodes to hit the disk */
	sync();

	/* create the handles */
	for (i=0; i < NUMFILES; i++) {
		sprintf(fname, "%s/file%06d", test_dir, i);
		ret = path_to_handle(fname, &handle[i], &hlen[i]);
		if (ret < 0) {
			perror("path_to_handle");
			return EXIT_FAILURE;
		}
	}

	/* unlink the files */
	for (i=0; i < NUMFILES; i++) {
		sprintf(fname, "%s/file%06d", test_dir, i);
		ret = unlink(fname);
		if (ret < 0) {
			perror("unlink");
			return EXIT_FAILURE;
		}
	}

	/* sync to get log forced for unlink transactions to hit the disk */
	sync();

	/* sync once more FTW */
	sync();

	/*
	 * now drop the caches so that unlinked inodes are reclaimed and
	 * buftarg page cache is emptied so that the inode cluster has to be
	 * fetched from disk again for the open_by_handle() call.
	 */
	system("echo 3 > /proc/sys/vm/drop_caches");

	/*
	 * now try to open the files by the stored handles. Expecting ENOENT
	 * for all of them.
	 */
	for (i=0; i < NUMFILES; i++) {
		errno = 0;
		fd = open_by_handle(handle[i], hlen[i], O_RDWR);
		if (fd < 0 && (errno == ENOENT || errno == ESTALE)) {
			free_handle(handle[i], hlen[i]);
			continue;
		}
		if (fd >= 0) {
			printf("open_by_handle(%d) opened an unlinked file!\n", i);
			close(fd);
		} else
			printf("open_by_handle(%d) returned %d incorrectly on an unlinked file!\n", i, errno);
		free_handle(handle[i], hlen[i]);
		failed++;
	}
	if (failed)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
