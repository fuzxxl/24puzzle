/*-
 * Copyright (c) 2017--2018 Robert Clausecker. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* pdbstats.c -- compute PDB statistics */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "pdb.h"

static void
usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-t tileset] [-p] pdbfile\n", argv0);

	exit(EXIT_FAILURE);
}

/*
 * Gather statistics about the PDB.  Store how often each entry occured
 * in histogram.  Return the number of bytes read from pdbfile.
 */
static off_t
gather_data(FILE *pdbfile, off_t histogram[PDB_HISTOGRAM_LEN])
{
	off_t size = 0;
	int c;

	flockfile(pdbfile);

	while (c = getc_unlocked(pdbfile), c != EOF) {
		size++;
		histogram[c]++;
	}

	funlockfile(pdbfile);
	fclose(pdbfile);
	return (size);
}

/*
 * Compute a histogram and print it.  Then, compute the entropy of the
 * PDB and print it.
 */
static void
print_histogram(off_t histogram[PDB_HISTOGRAM_LEN], off_t size)
{
	size_t i;
	double quotient = 1.0 / (double)size, entropy, bits, prob, accum = 0.0;

	printf("histogram:\n");

	for (i = 0; i < PDB_HISTOGRAM_LEN; i++) {
		if (histogram[i] == 0)
			continue;

		prob = histogram[i] * quotient;
		entropy = -log2(prob);
		bits = histogram[i] * entropy;
		accum += bits;

		printf("0x%02zx: %20llu * %6.2fb (%6.2f%%) = %23.2fb (%23.2fB)\n",
		    i, (unsigned long long)histogram[i], entropy, 100.0 * prob, bits, bits / 8);
	}

	printf("total %.2fb (%.2fB)\n\n", accum, accum / 8);
}

/*
 * Compute eta from the histogram and print it out.
 */
static void
print_eta(off_t histogram[PDB_HISTOGRAM_LEN], off_t size)
{
	double eta = 0.0, invb = 1.0 / B;
	size_t i;

	for (i = 1; i <= PDB_HISTOGRAM_LEN; i++)
		eta = eta * invb + (double)histogram[PDB_HISTOGRAM_LEN - i];

	eta /= (double)size;

	printf("eta = %.20e\n", eta);
}

/*
 * Print a single-line histogram as requested from the option -p.  This
 * is used to build the histograms.txt file in genallpdbs.sh.  The line
 * contains first the tile set, and then a space separated histogram,
 * ending with the first 0 entry.
 */
static void
histogram_line(const char *tsstr, off_t histogram[PDB_HISTOGRAM_LEN])
{
	size_t i;

	if (tsstr != NULL)
		printf("%s ", tsstr);

	for (i = 0; i < PDB_HISTOGRAM_LEN && histogram[i] != 0; i++)
		printf("%llu ", (unsigned long long)histogram[i]);

	printf("0\n");
}

extern int
main(int argc, char *argv[])
{
	FILE *pdbfile;
	off_t size, histogram[PDB_HISTOGRAM_LEN] = {};
	int single_line = 0, optchar;
	const char *tsstr = NULL;

	while (optchar = getopt(argc, argv, "t:p"), optchar != -1)
		switch (optchar) {
		case 'p':
			single_line = 1;
			break;

		case 't':
			tsstr = optarg;
			break;

		default:
			usage(argv[0]);
		}

	if (argc - optind != 1)
		usage(argv[0]);

	pdbfile = fopen(argv[optind], "rb");
	if (pdbfile == NULL) {
		perror("fopen");
		return (EXIT_FAILURE);
	}


	size = gather_data(pdbfile, histogram);

	if (single_line)
		histogram_line(tsstr, histogram);
	else {
		printf("size %zuB\n\n", size);
		print_histogram(histogram, size);
		print_eta(histogram, size);
	}

	return (EXIT_SUCCESS);
}
