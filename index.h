#ifndef INDEX_H
#define INDEX_H

#include "puzzle.h"
#include "tileset.h"

/*
 * To build pattern databases, we need a perfect bijective hash function
 * from partial puzzle configurations to integers in 0 ... n-1 where n
 * is the number of possible partial puzzle configurations for the given
 * tile set.  In this program, we use generalized Lehmer codes as our
 * index function.  The Lehmer code assigns to each tile the number of
 * higher numbered tiles before it.  When applying this to partial
 * puzzle configurations, we simply consider all ignored tiles to have
 * higher numbers than all tiles in our tile set.  This does the right
 * thing.  See the implementation for notes on how to efficiently
 * compute Lehmer codes.
 *
 * For many use cases, we are content with having the index
 * split up into its components, which is why we separate the
 * computation of the structured index (struct index) and the
 * index product (index).
 */
struct index {
	unsigned char cmp[25];
}

/*
 * This type represents an index into the pattern database.  Under a
 * suitable factorial number system, each index is equivalent to its
 * corresponding structured index.  You can use the functions
 * combine_index() and split_index() to convert between the two
 * representations.
 */

extern index	search_space_size(tileset);
extern void	compute_index(tileset, struct index*, const struct puzzle*);
extern void	invert_index(tileset, struct puzzle*, const struct index*);
extern index	combine_index(tileset, const struct index*);
extern void	split_index(tileset, struct index*, index);

#endif /* INDEX_H */
