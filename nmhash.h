// verification:
// rurban/smhasher: 0x0DC5C2DC
// demerphq/smhasher: 0xDF3272CF

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _nmhash_h_
#define _nmhash_h_

#define NMH_VERSION 0

#include <stdint.h>
#include <string.h>

#if defined(__GNUC__)
#  if defined(__AVX2__)
#    include <immintrin.h>
#  elif defined(__SSE2__)
#    include <emmintrin.h>
#  endif
#elif defined(_MSC_VER)
#  include <intrin.h>
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3))  \
  || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) \
  || defined(__clang__)
#    define NMH_likely(x) __builtin_expect(x, 1)
#else
#    define NMH_likely(x) (x)
#endif

#if ((defined(sun) || defined(__sun)) && __cplusplus) /* Solaris includes __STDC_VERSION__ with C++. Tested with GCC 5.5 */
#  define NMH_RESTRICT /* disable */
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* >= C99 */
#  define NMH_RESTRICT   restrict
#elif defined(__cplusplus) && (defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER))
#  define NMH_RESTRICT __restrict__
#elif defined(__cplusplus) && defined(_MSC_VER)
#  define NMH_RESTRICT __restrict
#else
#  define NMH_RESTRICT   /* disable */
#endif

// endian macros
#ifndef NMHASH_LITTLE_ENDIAN
#  if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define NMHASH_LITTLE_ENDIAN 1
#  elif defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define NMHASH_LITTLE_ENDIAN 0
#  else
#    warning could not determine endianness! Falling back to little endian.
#    define NMHASH_LITTLE_ENDIAN 1
#  endif
#endif

// vector macros
#define NMH_SCALAR 0
#define NMH_SSE2   1
#define NMH_AVX2   2
#define NMH_AVX512 3

#ifndef NMH_VECTOR    /* can be defined on command line */
#  if defined(__AVX512BW__)
#    define NMH_VECTOR NMH_AVX512 // _mm512_mullo_epi16 requires AVX512BW
#  elif defined(__AVX2__)
#    define NMH_VECTOR NMH_AVX2  // add '-mno-avx256-split-unaligned-load' and '-mn-oavx256-split-unaligned-store' for gcc
#  elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#    define NMH_VECTOR NMH_SSE2
#  else
#    define NMH_VECTOR NMH_SCALAR
#  endif
#endif

// align macros
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)   /* C11+ */
#  include <stdalign.h>
#  define NMH_ALIGN(n)      alignas(n)
#elif defined(__GNUC__)
#  define NMH_ALIGN(n)      __attribute__ ((aligned(n)))
#elif defined(_MSC_VER)
#  define NMH_ALIGN(n)      __declspec(align(n))
#else
#  define NMH_ALIGN(n)   /* disabled */
#endif

// constants

// primes from xxh
#define NMH_PRIME32_1  UINT32_C(0x9E3779B1)
#define NMH_PRIME32_2  UINT32_C(0x85EBCA77)
#define NMH_PRIME32_3  UINT32_C(0xC2B2AE3D)
#define NMH_PRIME32_4  UINT32_C(0x27D4EB2F)

/*! Pseudorandom secret taken directly from FARSH. */
NMH_ALIGN(64) static const uint32_t NMH_ACC_INIT[32] = {
	UINT32_C(0x71644897), UINT32_C(0xA20DF94E), UINT32_C(0x3819EF46), UINT32_C(0xA9DEACD8),
	UINT32_C(0xA8FA763F), UINT32_C(0xE39C343F), UINT32_C(0xF9DCBBC7), UINT32_C(0xC70B4F1D),
	UINT32_C(0x8A51E04B), UINT32_C(0xCDB45931), UINT32_C(0xC89F7EC9), UINT32_C(0xD9787364),

	UINT32_C(0xB8FE6C39), UINT32_C(0x23A44BBE), UINT32_C(0x7C01812C), UINT32_C(0xF721AD1C),
	UINT32_C(0xDED46DE9), UINT32_C(0x839097DB), UINT32_C(0x7240A4A4), UINT32_C(0xB7B3671F),
	UINT32_C(0xCB79E64E), UINT32_C(0xCCC0E578), UINT32_C(0x825AD07D), UINT32_C(0xCCFF7221),
	UINT32_C(0xB8084674), UINT32_C(0xF743248E), UINT32_C(0xE03590E6), UINT32_C(0x813A264C),
	UINT32_C(0x3C2852BB), UINT32_C(0x91C300CB), UINT32_C(0x88D0658B), UINT32_C(0x1B532EA3),
};

// read functions
static inline
uint64_t
NMH_readLE32(const void *const p)
{
	uint32_t v;
	memcpy(&v, p, 4);
#	if (NMHASH_LITTLE_ENDIAN)
	return v;
#	elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
	return __builtin_bswap32(v);
#	elif defined(_MSC_VER)
	return _byteswap_ulong(v);
#	else
	return ((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | ((v << 24) & 0xff000000);
#	endif
}

typedef enum { NMH_SHORT32_WITHOUT_SEED2=0, NMH_SHORT32_WITH_SEED2=1 } NMH_SHORT32_SEED2;

static inline
uint32_t
NMHASH32_short32(uint32_t const x, uint32_t const seed2, NMH_SHORT32_SEED2 const withSeed2)
{
	// score = 1.8481760596580992
#	define __NMH_M1 UINT32_C(0x3E550CF1)
#	define __NMH_M2 UINT32_C(0x9E1B7E49)

#	if NMH_VECTOR == NMH_SCALAR
	{
		union { uint32_t u32; uint16_t u16[2]; } vx = { .u32 = x };
		vx.u32 ^= (vx.u32 << 18) ^ (vx.u32 >> 22);
		vx.u32 ^= (vx.u32 >> 11) ^ (vx.u32 >> 13);
		vx.u16[0] *= (uint16_t)__NMH_M1;
		vx.u16[1] *= (uint16_t)(__NMH_M1 >> 16);
		if (NMH_SHORT32_WITH_SEED2 == withSeed2) {
			vx.u32 += seed2;
		}
		vx.u32 ^= (vx.u32 << 15) ^ ( vx.u32 >> 24);
		vx.u32 ^= (vx.u32 >> 6) ^ ( vx.u32 >> 21);
		vx.u16[0] *= (uint16_t)__NMH_M2;
		vx.u16[1] *= (uint16_t)(__NMH_M2 >> 16);
		vx.u32 ^= (vx.u32 << 7) ^ ( vx.u32 >> 13);
		vx.u32 ^= (vx.u32 << 15) ^ ( vx.u32 >> 11);
		return vx.u32;
	}
#	else // at least NMH_SSE2
	{
		__m128i hv = _mm_setr_epi32(x, 0, 0, 0);

		hv = _mm_xor_si128(_mm_xor_si128(hv, _mm_slli_epi32(hv, 18)), _mm_srli_epi32(hv, 22));
		hv = _mm_xor_si128(_mm_xor_si128(hv, _mm_srli_epi32(hv, 11)), _mm_srli_epi32(hv, 13));
		hv = _mm_mullo_epi16(hv, _mm_setr_epi32(__NMH_M1, 0, 0, 0));

		if (NMH_SHORT32_WITH_SEED2 == withSeed2) {
			hv = _mm_add_epi32(hv, _mm_setr_epi32(seed2, 0, 0, 0));
		}

		hv = _mm_xor_si128(_mm_xor_si128(hv, _mm_slli_epi32(hv, 15)), _mm_srli_epi32(hv, 24));
		hv = _mm_xor_si128(_mm_xor_si128(hv, _mm_srli_epi32(hv, 6)), _mm_srli_epi32(hv, 21));
		hv = _mm_mullo_epi16(hv, _mm_setr_epi32(__NMH_M2, 0, 0, 0));

		hv = _mm_xor_si128(_mm_xor_si128(hv, _mm_slli_epi32(hv, 7)), _mm_srli_epi32(hv, 13));
		hv = _mm_xor_si128(_mm_xor_si128(hv, _mm_slli_epi32(hv, 15)), _mm_srli_epi32(hv, 11));
		return *(uint32_t*)&hv;
	}
#	endif

#	undef __NMH_M1
#	undef __NMH_M2
}

typedef enum { NMH_AVALANCHE_FULL=0, NMH_AVALANCHE_INNER=1 } NMH_AVALANCHE;

#define __NMH_M3 UINT32_C(0xCCE5196D)
#define __NMH_M4 UINT32_C(0x464BE229)

static inline
uint32_t
NMHASH32_avalanche32(uint32_t const x, NMH_AVALANCHE const type)
{
	//[-21 -8 cce5196d 12 -7 464be229 -21 -8] = 3.2267098842182733
	union { uint32_t u32; uint16_t u16[2]; } vx = { .u32 = x };
	vx.u32    ^= (vx.u32 >> 8) ^ (vx.u32 >> 21);
	vx.u16[0] *= (uint16_t)__NMH_M3;
	vx.u16[1] *= (uint16_t)(__NMH_M3 >> 16);
	vx.u32    ^= (vx.u32 << 12) ^ (vx.u32 >> 7);
	vx.u16[0] *= (uint16_t)__NMH_M4;
	vx.u16[1] *= (uint16_t)(__NMH_M4 >> 16);
	if (NMH_AVALANCHE_FULL == type) {
		return vx.u32 ^ (vx.u32 >> 8) ^ (vx.u32 >> 21);
	}
	return vx.u32;
}

static inline
uint32_t
NMHASH32_mix32(uint32_t const h, uint32_t const x, NMH_AVALANCHE const type)
{
        return NMHASH32_avalanche32(h ^ x, type);
}

static inline
uint32_t
NMHASH32_5to127(const uint8_t* const NMH_RESTRICT p, size_t const len, uint32_t const seed, NMH_AVALANCHE const type)
{
#	if NMH_VECTOR == NMH_SCALAR
	{
		uint32_t a = NMH_PRIME32_1 + seed;
		uint32_t b = NMH_PRIME32_2 + seed;
		uint32_t c = NMH_PRIME32_3 + seed;
		uint32_t d = NMH_PRIME32_4 + seed;

		if (NMH_AVALANCHE_FULL == type) {
			// 17 to 127 bytes
			int const nbRounds = (int)(len - 1) / 16;
			int i;
			for (i = 0; i < nbRounds; ++i) {
				a = NMHASH32_mix32(a, NMH_readLE32(p + i * 16 + 0), NMH_AVALANCHE_INNER);
				b = NMHASH32_mix32(b, NMH_readLE32(p + i * 16 + 4), NMH_AVALANCHE_INNER);
				c = NMHASH32_mix32(c, NMH_readLE32(p + i * 16 + 8), NMH_AVALANCHE_INNER);
				d = NMHASH32_mix32(d, NMH_readLE32(p + i * 16 + 12), NMH_AVALANCHE_INNER);
			}
			a = NMHASH32_mix32(a, NMH_readLE32(p + len - 16 + 0), NMH_AVALANCHE_FULL);
			b = NMHASH32_mix32(b, NMH_readLE32(p + len - 16 + 4), NMH_AVALANCHE_FULL);
			c = NMHASH32_mix32(c, NMH_readLE32(p + len - 16 + 8), NMH_AVALANCHE_FULL);
			d = NMHASH32_mix32(d, NMH_readLE32(p + len - 16 + 12), NMH_AVALANCHE_FULL);
		} else {
			// 5 to 16 bytes
			a = NMHASH32_mix32(a, NMH_readLE32(p), NMH_AVALANCHE_FULL);
			b = NMHASH32_mix32(b, NMH_readLE32(p + ((len>>3)<<2)), NMH_AVALANCHE_FULL);
			c = NMHASH32_mix32(c, NMH_readLE32(p + len - 4), NMH_AVALANCHE_FULL);
			d = NMHASH32_mix32(d, NMH_readLE32(p + len - 4 - ((len>>3)<<2)), NMH_AVALANCHE_FULL);
		}

		a ^= NMH_PRIME32_1;
		b ^= NMH_PRIME32_2;
		c ^= NMH_PRIME32_3;
		d ^= NMH_PRIME32_4;

		return NMHASH32_mix32(a + b + c + d, (uint32_t)len + seed, NMH_AVALANCHE_FULL);
	}
#	else // at least NMH_SSE2
	{
		__m128i const h0 = _mm_setr_epi32(NMH_PRIME32_1, NMH_PRIME32_2, NMH_PRIME32_3, NMH_PRIME32_4);
		__m128i const m1 = _mm_set1_epi32(__NMH_M3);
		__m128i const m2 = _mm_set1_epi32(__NMH_M4);

		__m128i       acc  = _mm_add_epi32(h0, _mm_set1_epi32(seed));
		__m128i       data;

		if (NMH_AVALANCHE_FULL == type) {
			// 17 to 127 bytes
			int const nbRounds = (int)(len - 1) / 16;
			int i;
			for (i = 0; i < nbRounds; ++i) {
				data = _mm_loadu_si128((const __m128i *)(p + i * 16));
				acc = _mm_xor_si128(acc, data);
				acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_srli_epi32(acc, 8)), _mm_srli_epi32(acc, 21));
				acc = _mm_mullo_epi16(acc, m1);
				acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_slli_epi32(acc, 12)), _mm_srli_epi32(acc, 7));
				acc = _mm_mullo_epi16(acc, m2);
			}

			data = _mm_loadu_si128((const __m128i *)(p + len - 16));
		} else {
			// 5 to 16 bytes
			data = _mm_setr_epi32(
					NMH_readLE32(p),
					NMH_readLE32(p + ((len>>3)<<2)),
					NMH_readLE32(p + len - 4),
					NMH_readLE32(p + len - 4 - ((len>>3)<<2)));
		}


		acc = _mm_xor_si128(acc, data);
		acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_srli_epi32(acc, 8)), _mm_srli_epi32(acc, 21));
		acc = _mm_mullo_epi16(acc, m1);
		acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_slli_epi32(acc, 12)), _mm_srli_epi32(acc, 7));
		acc = _mm_mullo_epi16(acc, m2);
		acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_srli_epi32(acc, 8)), _mm_srli_epi32(acc, 21));

		acc = _mm_xor_si128(acc, h0);
		acc = _mm_add_epi32(acc, _mm_bsrli_si128(acc, 4));
		acc = _mm_add_epi32(acc, _mm_bsrli_si128(acc, 8));

		acc = _mm_xor_si128(acc, _mm_setr_epi32((uint32_t)len + seed, 0, 0, 0));
		acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_srli_epi32(acc, 8)), _mm_srli_epi32(acc, 21));
		acc = _mm_mullo_epi16(acc, m1);
		acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_slli_epi32(acc, 12)), _mm_srli_epi32(acc, 7));
		acc = _mm_mullo_epi16(acc, m2);
		acc = _mm_xor_si128(_mm_xor_si128(acc, _mm_srli_epi32(acc, 8)), _mm_srli_epi32(acc, 21));

		return *(uint32_t*)&acc;
	}
#	endif
}

NMH_ALIGN(64) static const uint32_t __NMH_M3_V[16] = {
	__NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3,
	__NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3, __NMH_M3,
};
NMH_ALIGN(64) static const uint32_t __NMH_M4_V[16] = {
	__NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4,
	__NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4, __NMH_M4,
};

#if NMH_VECTOR == NMH_SCALAR
#define NMHASH32_long_round NMHASH32_long_round_scalar
static inline
void
NMHASH32_long_round_scalar(uint32_t *const NMH_RESTRICT acc, const uint8_t* const NMH_RESTRICT p, const NMH_AVALANCHE type)
{
	// breadth first calculation will hint some compiler to auto vectorize the code
	// on gcc, the performance becomes 10x than the depth first, and about 80% of the manually vectorized code
	const size_t nbGroups = sizeof(NMH_ACC_INIT) / sizeof(*NMH_ACC_INIT);
	size_t i;

	for (i = 0; i < nbGroups; ++i) {
		acc[i] ^= NMH_readLE32(p + i * 4);
	}
	for (i = 0; i < nbGroups; ++i) {
		acc[i] ^= (acc[i] >> 8) ^ (acc[i] >> 21);
	}
	for (i = 0; i < nbGroups; ++i) {
		((uint16_t*)acc)[i] *= ((uint16_t*)__NMH_M3_V)[i];
	}
	for (i = 0; i < nbGroups; ++i) {
		((uint16_t*)acc)[nbGroups + i] *= ((uint16_t*)__NMH_M3_V)[i];
	}
	for (i = 0; i < nbGroups; ++i) {
		acc[i] ^= (acc[i] << 12) ^ (acc[i] >> 7);
	}
	for (i = 0; i < nbGroups; ++i) {
		((uint16_t*)acc)[i] *= ((uint16_t*)__NMH_M4_V)[i];
	}
	for (i = 0; i < nbGroups; ++i) {
		((uint16_t*)acc)[nbGroups + i] *= ((uint16_t*)__NMH_M4_V)[i];
	}

	if (NMH_AVALANCHE_FULL == type) {
		for (i = 0; i < nbGroups; ++i) {
			acc[i] ^= (acc[i] >> 8) ^ (acc[i] >> 21);
		}
	}
}
#endif

#define NMH_VECTOR_NB_GROUP (sizeof(NMH_ACC_INIT) / sizeof(*NMH_ACC_INIT) / (sizeof(vector_t) / sizeof(*NMH_ACC_INIT)))

#if NMH_VECTOR == NMH_SSE2
#define NMHASH32_long_round NMHASH32_long_round_sse2
static inline
void
NMHASH32_long_round_sse2(uint32_t *const NMH_RESTRICT acc, const uint8_t* const NMH_RESTRICT p, const NMH_AVALANCHE type)
{
	typedef __m128i vector_t;
	const vector_t *const NMH_RESTRICT m1   = (const vector_t * NMH_RESTRICT)__NMH_M3_V;
	const vector_t *const NMH_RESTRICT m2   = (const vector_t * NMH_RESTRICT)__NMH_M4_V;
	      vector_t *const              xacc = (      vector_t *             )acc;
	      vector_t *const              xp   = (      vector_t *             )p;
	      vector_t                     data[NMH_VECTOR_NB_GROUP];
	size_t i;

	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		data[i] = _mm_loadu_si128(xp + i);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm_xor_si128(xacc[i], data[i]);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm_xor_si128(_mm_xor_si128(xacc[i], _mm_srli_epi32(xacc[i], 8)), _mm_srli_epi32(xacc[i], 21));
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm_mullo_epi16(xacc[i], *m1);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm_xor_si128(_mm_xor_si128(xacc[i], _mm_slli_epi32(xacc[i], 12)), _mm_srli_epi32(xacc[i], 7));
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm_mullo_epi16(xacc[i], *m2);
	}

	if (NMH_AVALANCHE_FULL == type) {
		for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
			xacc[i] = _mm_xor_si128(_mm_xor_si128(xacc[i], _mm_srli_epi32(xacc[i], 8)), _mm_srli_epi32(xacc[i], 21));
		}
	}
}
#endif

#if NMH_VECTOR == NMH_AVX2
#define NMHASH32_long_round NMHASH32_long_round_avx2
static inline
void
NMHASH32_long_round_avx2(uint32_t *const NMH_RESTRICT acc, const uint8_t* const NMH_RESTRICT p, const NMH_AVALANCHE type)
{
	typedef __m256i vector_t;
	const vector_t *const NMH_RESTRICT m1   = (const vector_t * NMH_RESTRICT)__NMH_M3_V;
	const vector_t *const NMH_RESTRICT m2   = (const vector_t * NMH_RESTRICT)__NMH_M4_V;
	      vector_t *const              xacc = (      vector_t *             )acc;
	      vector_t *const              xp   = (      vector_t *             )p;
	      vector_t                     data[NMH_VECTOR_NB_GROUP];
	size_t i;

	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		data[i] = _mm256_loadu_si256(xp + i);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm256_xor_si256(xacc[i], data[i]);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm256_xor_si256(_mm256_xor_si256(xacc[i], _mm256_srli_epi32(xacc[i], 8)), _mm256_srli_epi32(xacc[i], 21));
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm256_mullo_epi16(xacc[i], *m1);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm256_xor_si256(_mm256_xor_si256(xacc[i], _mm256_slli_epi32(xacc[i], 12)), _mm256_srli_epi32(xacc[i], 7));
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm256_mullo_epi16(xacc[i], *m2);
	}

	if (NMH_AVALANCHE_FULL == type) {
		for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
			xacc[i] = _mm256_xor_si256(_mm256_xor_si256(xacc[i], _mm256_srli_epi32(xacc[i], 8)), _mm256_srli_epi32(xacc[i], 21));
		}
	}
}
#endif

#if NMH_VECTOR == NMH_AVX512
#define NMHASH32_long_round NMHASH32_long_round_avx512
static inline
void
NMHASH32_long_round_avx512(uint32_t *const NMH_RESTRICT acc, const uint8_t* const NMH_RESTRICT p, const NMH_AVALANCHE type)
{
	typedef __m512i vector_t;
	const vector_t *const NMH_RESTRICT m1   = (const vector_t * NMH_RESTRICT)__NMH_M3_V;
	const vector_t *const NMH_RESTRICT m2   = (const vector_t * NMH_RESTRICT)__NMH_M4_V;
	      vector_t *const              xacc = (      vector_t *             )acc;
	      vector_t *const              xp   = (      vector_t *             )p;
	      vector_t                     data[NMH_VECTOR_NB_GROUP];
	size_t i;

	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		data[i] = _mm512_loadu_si512(xp + i);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm512_xor_si512(xacc[i], data[i]);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm512_xor_si512(_mm512_xor_si512(xacc[i], _mm512_srli_epi32(xacc[i], 8)), _mm512_srli_epi32(xacc[i], 21));
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm512_mullo_epi16(xacc[i], *m1);
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm512_xor_si512(_mm512_xor_si512(xacc[i], _mm512_slli_epi32(xacc[i], 12)), _mm512_srli_epi32(xacc[i], 7));
	}
	for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
		xacc[i] = _mm512_mullo_epi16(xacc[i], *m2);
	}

	if (NMH_AVALANCHE_FULL == type) {
		for (i = 0; i < NMH_VECTOR_NB_GROUP; ++i) {
			xacc[i] = _mm512_xor_si512(_mm512_xor_si512(xacc[i], _mm512_srli_epi32(xacc[i], 8)), _mm512_srli_epi32(xacc[i], 21));
		}
	}
}
#endif

static inline
uint32_t
NMHASH32_merge_acc(uint32_t *const NMH_RESTRICT acc, const size_t len)
{
	int i, sum = (uint32_t)(len >> 32);
	for (i = 0; i < sizeof(NMH_ACC_INIT)/sizeof(*NMH_ACC_INIT); ++i) {
		acc[i] ^= NMH_ACC_INIT[i];
	}
	for (i = 0; i < sizeof(NMH_ACC_INIT)/sizeof(*NMH_ACC_INIT); ++i) {
		sum += acc[i];
	}
	return NMHASH32_mix32(sum, (uint32_t)len, NMH_AVALANCHE_FULL);
}

static
uint32_t
NMHASH32_long(const uint8_t* const NMH_RESTRICT p, size_t const len, uint32_t const seed)
{
	NMH_ALIGN(64) uint32_t acc[sizeof(NMH_ACC_INIT)/sizeof(*NMH_ACC_INIT)];
	size_t const nbRounds = (len - 1) / sizeof(acc);
	size_t i;

	// init
	for (i = 0; i < sizeof(NMH_ACC_INIT)/sizeof(*NMH_ACC_INIT); ++i) {
		acc[i] = NMH_ACC_INIT[i] + seed;
	}

	for (i = 0; i < nbRounds; ++i) {
		NMHASH32_long_round(acc, p + i * sizeof(acc), NMH_AVALANCHE_INNER);
	}
	NMHASH32_long_round(acc, p + len - sizeof(acc), NMH_AVALANCHE_FULL);

	return NMHASH32_merge_acc(acc, len);
}

#undef __NMH_M3
#undef __NMH_M4
#undef NMH_VECTOR_NB_GROUP

static inline
uint32_t
NMHASH32(const void* const NMH_RESTRICT input, size_t const len, uint32_t const seed)
{
	const uint8_t *const p = (const uint8_t *)input;
	if (NMH_likely(len <= 16)) {
		if(NMH_likely(len > 4)) {
			return NMHASH32_5to127(p, len, seed, NMH_AVALANCHE_INNER);
		}
		if (NMH_likely(len > 0)) {
			union { uint32_t u32; uint8_t u8[4]; } x = { .u32 = p[0] };
			x.u8[1] = p[len-1];
			x.u32 <<= 16;
			x.u8[0] = p[len>>1];
			x.u8[1] = p[len>>2];
			return -NMHASH32_short32((NMH_PRIME32_4 + seed) ^ x.u32, (uint32_t)len ^ seed, NMH_SHORT32_WITH_SEED2);
                }
		return NMHASH32_short32(NMH_PRIME32_1 + seed, 0, NMH_SHORT32_WITHOUT_SEED2);
	}
	if (NMH_likely(len < 128)) {
		return NMHASH32_5to127(p, len, seed, NMH_AVALANCHE_FULL);
	}
	return NMHASH32_long(p, len, seed);
}

#endif /* _nmhash_h_ */

#ifdef __cplusplus
}
#endif