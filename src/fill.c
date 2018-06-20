// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2000-2001 Silicon Graphics, Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * fill pathname key nbyte
 *
 * use key to seed random number generator so data is deterministic
 *
 * lines are at most 72 bytes long so diff(1) does not choke
 */

int
main(int argc, char **argv)
{
    FILE		*f;
    unsigned int	seed;
    long		i;
    int			nbyte;
    char		*hdr;
    char		*hp;
    char		c;

    /* quick and dirty, no args checking */
    if ((f = fopen(argv[1], "w")) == NULL) {
	fprintf(stderr, "fill: cannot create \"%s\": %s\n", argv[1], strerror(errno));
	exit(1);
    }
    seed = 0;
    i = 0;
    while (argv[2][i]) {
	seed <<= 8;
	seed |= argv[2][i];
	i++;
    }
    srand(seed);

    nbyte = atoi(argv[3]);

    /*
     * line format
     *
     * byte offset @ start of this line XXXXXXXXXXXX
     * test iteration number XXXX
     * key (usually file name) argv[2]
     * random bytes to fill the line
     */

    hdr = (char *)malloc(12+1+4+1+strlen(argv[2])+1+1);
    sprintf(hdr, "%012ld %04d %s ", (long int)0, 0, argv[2]);
    hp = hdr;

    for (i = 0; i < nbyte-1; i++) {
	if (*hp) {
	    c = *hp;
	    hp++;
	}
	else if ((i % 72) == 71)
	    c = '\n';
	else
	    c = 32+(rand() % (128-32));
	fputc(c, f);
	if (c == '\n') {
	    hp = hdr;
	    sprintf(hdr, "%012ld %04d %s ", i+1, 0, argv[2]);
	}
    }
    fputc('\n', f);

    exit(0);
}
