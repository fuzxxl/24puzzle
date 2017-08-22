/* rank.c -- tileset ranking and unranking */

#include <stdlib.h>
#include <stdio.h>

#include "puzzle.h"
#include "tileset.h"

/*
 * These tables store lookup tables for ranking positions.  Since we
 * typically only want to rank for one specific tile count, it is a
 * sensible choice to initialize the tables only as needed using
 * dynamic memory allocation.
 */
const tileset *unrank_tables[TILE_COUNT + 1] = {};

/*
 * Number of combinations for k items out of TILE_COUNT.  This is just
 * (TILE_COUNT choose k).  This lookup table is used to compute the
 * table sizes in tileset_unrank_init().  Note that the table is
 * symmetric, allowing us to omit half of it.
 */
static const unsigned
combination_count[(TILE_COUNT + 1) / 2] = {
	1,
	25,
	300,
	2300,
	12650,
	53130,
	177100,
	480700,
	1081575,
	2042975,
	3268760,
	4457400,
	5200300,
};


/*
 * Compute the lexicographically next combination with tileset_count(ts)
 * bits to ts and return it.
 */
static tileset
next_combination(tileset ts)
{
	/* https://graphics.stanford.edu/~seander/bithacks.html */
	tileset t = ts | ts - 1;

	return (t + 1 | (~t & -~t) - 1 >> tileset_get_least(ts) + 1);
}

/*
 * Allocate and initialize the unrank table for k bits out of
 * TILE_COUNT.  If memory allocation fails, abort the program.
 */
extern void
tileset_unrank_init(size_t k)
{
	size_t i, n = combination_count[k > TILE_COUNT / 2 ? TILE_COUNT - k : k];
	tileset iter, *tbl;

	tbl = malloc(n * sizeof *unrank_tables[k]);
	if (tbl == NULL) {
		perror("malloc");
		abort();
	}

	for (i = 0, iter = (1 << k + 1) - 1; i < n; i++, iter = next_combination(iter))
		tbl[i] = iter;

	unrank_tables[k] = tbl;
}
