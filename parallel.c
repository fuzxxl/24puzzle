/* parallel.c -- multi-threaded operation on pattern databases */

#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include "tileset.h"
#include "index.h"
#include "parallel.h"
#include "pdb.h"

int pdb_jobs = 1;

/*
 * This function is the main function of each worker thread.  It grabs
 * chunks off the pile and calls worker() for them until no work is
 * left.
 */
static void *
parallel_worker(void *cfgarg)
{
	struct parallel_config *cfg = cfgarg;
	cmbindex i, n;

	for (;;) {
		/* pick up chunk */
		i = atomic_fetch_add(&cfg->offset, PDB_CHUNK_SIZE);

		/* any work left to do? */
		if (i >= cfg->pdb_size)
			break;

		n = i + PDB_CHUNK_SIZE <= cfg->pdb_size ? PDB_CHUNK_SIZE : cfg->pdb_size - i;
		cfg->worker(cfgarg, i, n);
	}

	return (NULL);
}

/*
 * Iterate through the PDB in parallel.  Only the members pdb, ts, and
 * worker of cfg must be filled in, the other members are filled in by
 * the function.  If you want to pass extra data to cfg->worker, make
 * *cfg the first member of a larger structure as cfg is passed to every
 * call of worker.
 */
extern void
pdb_iterate_parallel(struct parallel_config *cfg)
{
	pthread_t pool[PDB_MAX_JOBS];

	int i, jobs = pdb_jobs, error;

	cfg->offset = 0;
	cfg->pdb_size = search_space_size(cfg->ts);

	/* for easier debugging, don't multithread when jobs == 1 */
	if (jobs == 1) {
		cfg->worker(cfg, 0, cfg->pdb_size);
		return;
	}

	/* spawn threads */
	for (i = 0; i < jobs; i++) {
		error = pthread_create(pool + i, NULL, parallel_worker, cfg);
		if (error == 0)
			continue;

		errno = error;
		perror("pthread_create");

		/*
		 * if we cannot spawn as many threads as we like
		 * but we can at least spawn some threads, just keep
		 * going with the tablebase generation.  Otherwise,
		 * there is no point in going on, so this is a good spot
		 * to throw our hands up in the air and give up.
		 */
		if (i++ > 0)
			break;

		fprintf(stderr, "Couldn't create any threads, aborting...\n");
		abort();
	}

	/* reduce count in case we couldn't create as many threads as we wanted */
	jobs = i;

	/* collect threads */
	for (i = 0; i < jobs; i++) {
		error = pthread_join(pool[i], NULL);
		if (error == 0)
			continue;

		errno = error;
		perror("pthread_join");
		abort();
	}
}