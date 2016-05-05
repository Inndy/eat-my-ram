/*  Written in 2016 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include <stdint.h>

/* This is the successor to xorshift128+. It is the fastest full-period
   generator passing BigCrush without systematic failures, but due to the
   relatively short period it is acceptable only for applications with a
   mild amount of parallelism; otherwise, use a xorshift1024* generator.

   Beside passing BigCrush, this generator passes the PractRand test suite
   up to (and included) 16TB, with the exception of binary rank tests,
   which fail due to the lowest bit being an LFSR; all other bits pass all
   tests.
   
   Note that the generator uses a simulated rotate operation, which most C
   compilers will turn into a single instruction. In Java, you can use
   Long.rotateLeft(). In languages that do not make low-level rotation
   instructions accessible xorshift128+ could be faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill xs128p_state. */

uint64_t xs128p_state[2];

static inline uint64_t xs128p_rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

int xs128p_seed(uint64_t a, uint64_t b) {
    if(a && b) {
        xs128p_state[0] = a;
        xs128p_state[1] = b;
        return 1;
    }
    return 0;
}

static inline uint64_t xs128p_next(void) {
	const uint64_t s0 = xs128p_state[0];
	uint64_t s1 = xs128p_state[1];
	const uint64_t result = s0 + s1;

	s1 ^= s0;
	xs128p_state[0] = xs128p_rotl(s0, 55) ^ s1 ^ (s1 << 14); // a, b
	xs128p_state[1] = xs128p_rotl(s1, 36); // c

	return result;
}


/* This is the xs128p_jump function for the generator. It is equivalent
   to 2^64 calls to xs128p_next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */

void xs128p_jump(void) {
	static const uint64_t JUMP[] = { 0xbeac0467eba5facb, 0xd86b048b86aa9922 };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	for(int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
		for(int b = 0; b < 64; b++) {
			if (JUMP[i] & 1ULL << b) {
				s0 ^= xs128p_state[0];
				s1 ^= xs128p_state[1];
			}
			xs128p_next();
		}

	xs128p_state[0] = s0;
	xs128p_state[1] = s1;
}
