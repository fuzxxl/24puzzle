/*-
 * Copyright (c) 2018, 2020 Robert Clausecker. All rights reserved.
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

/* compact.c -- compact puzzle representation */

#ifndef COMPACT_H
#define COMPACT_H

#include <stdlib.h> /* for free() */
#include "puzzle.h"

/*
 * To save storage, we store puzzles using a compact representation with
 * five bits per entry, not storing the position of the zero tile.
 * Additionally, four bits are used to store all moves that lead back to
 * the previous generation.  This leads to 24 * 5 + 4 = 124 bits of
 * storage being required in total, split into two 64 bit quantities.
 * lo and hi store 12 tiles @ 5 bits each, lo additionally stores 4 move
 * mask bits in its least significant bits.
 */
struct compact_puzzle {
	unsigned long long lo, hi;
};

#define MOVE_MASK 0xfull

/*
 * An array of struct compact_puzzle with the given length and capacity.
 */
struct cp_slice {
	struct compact_puzzle *data;
	size_t len, cap;
};

/* compact.c */
extern void	pack_puzzle(struct compact_puzzle *restrict, const struct puzzle *restrict);
extern void	pack_puzzle_masked(struct compact_puzzle *restrict, const struct puzzle *restrict, int);
extern void	unpack_puzzle(struct puzzle *restrict, const struct compact_puzzle *restrict);
extern int	compare_cp(const void *, const void *);
extern int	compare_cp_nomask(const void *, const void *);

extern void	cps_append(struct cp_slice *, const struct compact_puzzle *);
extern void	cps_round(struct cp_slice *restrict, const struct cp_slice *restrict);

/*
 * Initialize the content of cps to an empty slice.
 */
static inline void
cps_init(struct cp_slice *cps)
{
	cps->data = NULL;
	cps->len = 0;
	cps->cap = 0;
}

/*
 * Release all storage associated with cps.  The content of cps is
 * undefined afterwards.
 */
static inline void
cps_free(struct cp_slice *cps)
{
	free(cps->data);
}

/*
 * compute the move mask, which is a bit mask of four bits, indicating
 * with 1 every move that leads to a configuration in the previous
 * round.  This is used to avoid going back to the configuration we came
 * from in breadth first search.
 */
static inline int
move_mask(const struct compact_puzzle *cp)
{
	return (cp->lo & MOVE_MASK);
}

/*
 * clear the bits in the move mask
 */
static inline void
clear_move_mask(struct compact_puzzle *cp)
{
	cp->lo &= ~MOVE_MASK;
}

#endif /* COMPACT_H */
