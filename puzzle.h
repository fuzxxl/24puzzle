#ifndef PUZZLE_H
#define PUZZLE_H

#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>

/*
 * One configuration of a 24 puzzle.  A 24 puzzle configuration
 * comprises 24 tiles labelled 1 to 24 arranged in a 5x5 grid with one
 * spot remaining empty.  The goal of the puzzle is to arrange the
 * tiles like on the left:
 *
 *     []  1  2  3  4       1  2  3  4  5
 *      5  6  7  8  9       6  7  8  9 10
 *     10 11 12 13 14      11 12 13 14 15
 *     15 16 17 18 19      16 17 18 19 20
 *     20 21 22 23 24      21 22 23 24 []
 *
 * Note that this arrangement is different from the traditional
 * arrangement on the right.  It is however isomorphic to the
 * traditional tile arrangement by changing coordinates and tile
 * numbers.
 *
 * To simplify the algorithms we want to run on them, puzzle
 * configurations are stored in two ways: First, the position of
 * each tile is stored in tiles[], then, the tile on each grid
 * position is stored in grid[] with 0 indicating the empty spot.
 * If viewed as permutations of { 0, ..., 24 }, tiles[] and grid[]
 * are inverse to each other at any given time.
 */
enum { TILE_COUNT = 25, ZERO_TILE = 0 };

struct puzzle {
	alignas(8) unsigned char tiles[TILE_COUNT], grid[TILE_COUNT];
};

/* puzzle.c */
extern const struct puzzle solved_puzzle;
extern const signed char movetab[TILE_COUNT][4];
extern const unsigned char transpositions[TILE_COUNT];

enum { PUZZLE_STR_LEN = 151 };

extern void	transpose(struct puzzle *);
extern void	puzzle_string(char[PUZZLE_STR_LEN], const struct puzzle *);
extern int	puzzle_parse(struct puzzle *, const char *);
/*
 * Return the location of the zero tile in p.
 */
static inline size_t
zero_location(const struct puzzle *p)
{
	return (p->tiles[ZERO_TILE]);
}

/*
 * Move the empty square to dloc, modifying p.  It is not tested whether
 * dest is adjacent to the empty square's current location.  Furtermore,
 * this function assumes 0 <= dloc < 25.
 */
static inline void
move(struct puzzle *p, size_t dloc)
{
	size_t zloc, dtile;

	dtile = p->grid[dloc];
	zloc = zero_location(p);

	p->grid[dloc] = ZERO_TILE;
	p->grid[zloc] = dtile;

	p->tiles[dtile] = zloc;
	p->tiles[ZERO_TILE] = dloc;
}

/*
 * Return the number of moves when the empty square is at z.  It is
 * assumed that 0 <= z < 25.
 */
static inline size_t
move_count(size_t z)
{
	/*
	 * 0xefffee is 01110 11111 11111 11111 01110,
	 * 0x07e9c0 is 00000 01110 01110 01110 00000,
	 * i.e. everything but the corners and everything but the border.
	 */
	return (2 + ((0xefffee & 1 << z) != 0) + ((0x0739c0 & 1 << z) != 0));
}

/*
 * Return the possible moves from square z.  Up to four moves are
 * possible, the exact number can be found using move_count(z).  If
 * less than four moves are possible, the last one or two entries are
 * marked with -1.  It is assumed that 0 <= z < 25.
 */
static inline const signed char *
get_moves(size_t z)
{
	return (movetab[z]);
}

/* validation.c */
extern int	puzzle_valid(const struct puzzle *);

/* random.c */
extern atomic_ullong	random_seed;
extern void	random_puzzle(struct puzzle *);

#endif /* PUZZLE_H */
