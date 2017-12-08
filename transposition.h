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

#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <stdalign.h>

#include "tileset.h"
#include "puzzle.h"

enum {
	/*
	 * The number of ways the tray can be rotated and transposed.
	 */
	AUTOMORPHISM_COUNT = 2 * 4,
};

extern alignas(64) const unsigned char automorphisms[AUTOMORPHISM_COUNT][2][32];
/* transposition of the tray along the main diagonal */
#define transpositions (automorphisms[4][0])

extern void	transpose(struct puzzle *);
extern void	morph(struct puzzle *, unsigned);
extern tileset	tileset_morph(tileset, unsigned);
extern int	is_admissible_morphism(tileset, unsigned);
extern unsigned	canonical_automorphism(tileset);
extern unsigned	compose_morphisms(unsigned, unsigned);

/*
 * Invert an automorphism.  All automorphisms are self-inverse except
 * for 1 and 3 which are inverse to each other.
 */
static inline unsigned
inverse_morphism(unsigned a)
{
	return ((a | 2) == 3 ? a ^ 2 : a);
}

#endif /* TRANSPOSITION_H */
