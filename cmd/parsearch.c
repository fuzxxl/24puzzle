/*-
 * Copyright (c) 2017 Robert Clausecker. All rights reserved.
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

/* parsearch.c -- use the same PDBs to search for multiple puzzles at once */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "search.h"
#include "catalogue.h"
#include "fsm.h"
#include "pdb.h"
#include "index.h"
#include "puzzle.h"
#include "tileset.h"

enum { CHUNK_SIZE = 1024 };

struct psearch_config {
	pthread_mutex_t lock;
	FILE *puzzles;
	struct pdb_catalogue *cat;
	const struct fsm *fsm;
	int idaflags;
};

static void *
lookup_worker(void *cfgarg)
{
	struct psearch_config *cfg = cfgarg;
	struct puzzle p;
	struct path path;
	unsigned long long expansions;
	int error;
	char linebuf[BUFSIZ], *line;

	for (;;) {
		error = pthread_mutex_lock(&cfg->lock);
		if (error != 0) {
			errno = error;
			perror("pthread_mutex_lock");
			abort();
		}

		line = fgets(linebuf, BUFSIZ, cfg->puzzles);
		error = pthread_mutex_unlock(&cfg->lock);
		if (error != 0) {
			errno = error;
			perror("pthread_mutex_unlock");
			abort();
		}

		if (line == NULL)
			return (NULL);

		if (puzzle_parse(&p, linebuf) != 0) {
			fprintf(stderr, "Invalid puzzle, ignoring: %s", linebuf);
			continue;
		}

		expansions = search_ida(cfg->cat, cfg->fsm, &p, &path, NULL, NULL, cfg->idaflags);
		linebuf[strcspn(linebuf, "\n")] = '\0';
		flockfile(stdout);
		printf("%s %3zu %12llu ", linebuf, path.pathlen, expansions);
		path_string(linebuf, &path);
		printf("%s\n", linebuf);
		funlockfile(stdout);
	}
}

/*
 * Read puzzles from puzzles and look them up in cat, using fsm for
 * pruning.  Use up to pdb_threads job to do that.  Print solutions and
 * node counts to stdout.
 */
static void
lookup_multiple(struct pdb_catalogue *cat, const struct fsm *fsm,
    FILE *puzzles, int idaflags)
{
	struct psearch_config cfg;
	pthread_t pool[PDB_MAX_JOBS];
	int j, jobs = pdb_jobs, error;

	cfg.puzzles = puzzles;
	cfg.cat = cat;
	cfg.fsm = fsm;
	cfg.idaflags = idaflags;
	error = pthread_mutex_init(&cfg.lock, NULL);
	if (error != 0) {
		errno = error;
		perror("pthread_mutex_init");
		abort();
	}

	if (jobs == 1) {
		lookup_worker(&cfg);
		return;
	}

	for (j = 0; j < pdb_jobs; j++) {
		error = pthread_create(pool + j, NULL, lookup_worker, &cfg);
		if (error == 0)
			continue;

		errno = error;
		perror("pthread_create");

		if (j++ > 0)
			break;

		fprintf(stderr, "Couldn't create any threads, aborthing...\n");
		abort();
	}

	jobs = j;

	for (j = 0; j < jobs; j++) {
		error = pthread_join(pool[j], NULL);
		if (error == 0)
			continue;

		errno = error;
		perror("pthread_join");
		abort();
	}
}

static void
usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-Fit] [-j nproc] [-m fsmfile] [-d pdbdir] catalogue puzzles\n", argv0);

	exit(EXIT_FAILURE);
}

extern int
main(int argc, char *argv[])
{
	struct pdb_catalogue *cat;
	const struct fsm *fsm = &fsm_simple, *newfsm;
	FILE *puzzles, *fsmfile;
	int optchar, catflags = 0, idaflags = 0, transpose = 0;
	char *pdbdir = NULL;

	while (optchar = getopt(argc, argv, "Fd:ij:m:t"), optchar != -1)
		switch (optchar) {
		case 'F':
			idaflags |= IDA_LAST_FULL;
			break;

		case 'd':
			pdbdir = optarg;
			break;

		case 'i':
			catflags |= CAT_IDENTIFY;
			break;

		case 'j':
			pdb_jobs = atoi(optarg);
			if (pdb_jobs < 1 || pdb_jobs > PDB_MAX_JOBS) {
				fprintf(stderr, "Number of threads must be between 1 and %d\n",
				    PDB_MAX_JOBS);
				return (EXIT_FAILURE);
			}

			break;

		case 'm':
			fsmfile = fopen(optarg, "rb");
			if (fsmfile == NULL) {
				perror(optarg);
				fprintf(stderr, "Proceeding anyway...\n");
				break;
			}

			newfsm = fsm_load(fsmfile);
			if (fsm == NULL) {
				perror("fsm_load");
				fprintf(stderr, "Proceeding anyway...\n");
			} else
				fsm = newfsm;

			fclose(fsmfile);
			break;


		case 't':
			transpose = 0;
			break;

		default:
			usage(argv[0]);
		}

	if (argc != optind + 2)
		usage(argv[0]);

	cat = catalogue_load(argv[optind], pdbdir, catflags, NULL);
	if (cat == NULL) {
		perror("catalogue_load");
		return (EXIT_FAILURE);
	}

	if (transpose && catalogue_add_transpositions(cat) != 0) {
		perror("catalogue_add_transpositions");
		fprintf(stderr, "Proceeding anyway...\n");
	}

	puzzles = fopen(argv[optind + 1], "r");
	if (puzzles == NULL) {
		perror("fopen");
		return (EXIT_FAILURE);
	}

	/*
	 * Searching for solutions takes a while.  When output is
	 * redirected to a file with normal buffering, nothing is seen
	 * for quite some time.  By setting the buffering mode to line
	 * buffering, we see results as soon as they are generated.
	 */
	setvbuf(stdout, NULL, _IOLBF, 0);

	lookup_multiple(cat, fsm, puzzles, idaflags);

	return (EXIT_SUCCESS);
}
