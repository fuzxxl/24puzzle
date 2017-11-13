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

/* transposition.c -- transpose puzzles */

#ifdef __SSSE3__
# include <immintrin.h>
#endif

#include "puzzle.h"
#include "transposition.h"

/*
 * The grid locations transposed along the primary diagonal.
 */
const unsigned char transpositions[TILE_COUNT] = {
	 0,  5, 10, 15, 20,
	 1,  6, 11, 16, 21,
	 2,  7, 12, 17, 22,
	 3,  8, 13, 18, 23,
	 4,  9, 14, 19, 24,
};

#ifdef __AVX2__
/*
 * Compose permutations p and q using pshufb.  As pshufb permutes
 * within lanes, we need to split the composition into two shuffles and
 * then a recombination step.
 */
static __m256i
compose_avx(__m256i p, __m256i q)
{
	__m256i fifteen = _mm256_set1_epi8(15), sixteen = _mm256_set1_epi8(16);
	__m256i plo, phi, qlo, qhi;

	plo = _mm256_permute2x128_si256(p, p, 0x00);
	phi = _mm256_permute2x128_si256(p, p, 0x11);

	qlo = _mm256_or_si256(q, _mm256_cmpgt_epi8(q, fifteen));
	qhi = _mm256_sub_epi8(q, sixteen);

	return (_mm256_or_si256(_mm256_shuffle_epi8(plo, qlo), _mm256_shuffle_epi8(phi, qhi)));
}
#elif defined(__SSSE3__)
/*
 * Compose permutations p and q using pshufb.  This is similar to
 * compose_avx, but we store the parts to rlo and rhi as we cannot
 * return more than one value in C.
 */
static void
compose_sse(__m128i *rlo, __m128i *rhi, __m128i plo, __m128i phi, __m128i qlo, __m128i qhi)
{
	__m128i fifteen = _mm_set1_epi8(15), sixteen = _mm_set1_epi8(16);
	__m128i qlolo, qlohi, qhilo, qhihi;

	qlolo = _mm_or_si128(qlo, _mm_cmpgt_epi8(qlo, fifteen));
	qhilo = _mm_or_si128(qhi, _mm_cmpgt_epi8(qhi, fifteen));

	qlohi = _mm_sub_epi8(qlo, sixteen);
	qhihi = _mm_sub_epi8(qhi, sixteen);

	*rlo = _mm_or_si128(_mm_shuffle_epi8(plo, qlolo), _mm_shuffle_epi8(phi, qlohi));
	*rhi = _mm_or_si128(_mm_shuffle_epi8(plo, qhilo), _mm_shuffle_epi8(phi, qhihi));
}
#endif

/*
 * Transpose p along the main diagonal.  If * is the composition of
 * permutations, the following operation is performed:
 *
 *     grid = transpositions * grid * transpositions
 *     tiles = transpositions * tiles * transpositions
 *
 * Note that this simple formula obtains as transpositions is an
 * involution.  p and its transposition have the same distance to the
 * solved puzzle by construction, so we can lookup both a puzzle and its
 * transposition in the PDB and take the maximum of the two values to
 * get a better heuristic.
 */
extern void
transpose(struct puzzle *p)
{
#ifdef __AVX2__
	/* transposition mask */
	__m256i tmask = _mm256_setr_epi8(
	     0,  5, 10, 15, 20,
	     1,  6, 11, 16, 21,
	     2,  7, 12, 17, 22,
	     3,  8, 13, 18, 23,
	     4,  9, 14, 19, 24,
	    -1, -1, -1, -1, -1, -1, -1);

	__m256i tiles = _mm256_loadu_si256((__m256i*)p->tiles);
	tiles = compose_avx(tmask, compose_avx(tiles, tmask));
	_mm256_storeu_si256((__m256i*)p->tiles, tiles);

	__m256i grid = _mm256_loadu_si256((__m256i*)p->grid);
	grid = compose_avx(tmask, compose_avx(grid, tmask));
	_mm256_storeu_si256((__m256i*)p->grid, grid);
#elif defined(__SSSE3__)
	/* transposition mask */
	__m128i tmasklo = _mm_setr_epi8( 0,  5, 10, 15, 20,  1,  6, 11, 16, 21,  2,  7, 12, 17, 22,  3);
	__m128i tmaskhi = _mm_setr_epi8( 8, 13, 18, 23,  4,  9, 14, 19, 24, -1, -1, -1, -1, -1, -1, -1);

	__m128i tileslo = _mm_loadu_si128((__m128i*)p->tiles + 0);
	__m128i tileshi = _mm_loadu_si128((__m128i*)p->tiles + 1);
	compose_sse(&tileslo, &tileshi, tileslo, tileshi, tmasklo, tmaskhi);
	compose_sse(&tileslo, &tileshi, tmasklo, tmaskhi, tileslo, tileshi);
	_mm_storeu_si128((__m128i*)p->tiles + 0, tileslo);
	_mm_storeu_si128((__m128i*)p->tiles + 1, tileshi);

	__m128i gridlo = _mm_loadu_si128((__m128i*)p->grid + 0);
	__m128i gridhi = _mm_loadu_si128((__m128i*)p->grid + 1);
	compose_sse(&gridlo, &gridhi, gridlo, gridhi, tmasklo, tmaskhi);
	compose_sse(&gridlo, &gridhi, tmasklo, tmaskhi, gridlo, gridhi);
	_mm_storeu_si128((__m128i*)p->grid + 0, gridlo);
	_mm_storeu_si128((__m128i*)p->grid + 1, gridhi);
#else
	size_t i;

	for (i = 0; i < TILE_COUNT; i++) {
		p->tiles[i] = transpositions[p->tiles[transpositions[i]]];
		p->grid[p->tiles[i]] = i;
	}
#endif
}