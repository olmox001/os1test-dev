/*
 * kernel/libkernel/include/libkernel/math.h
 * Fixed-point math (16.16) — no FPU required.
 */
#ifndef _LIBKERNEL_MATH_H
#define _LIBKERNEL_MATH_H

#include <libkernel/types.h>

/* Fixed-point constants (16.16) */
#define FP_SHIFT 16
#define FP_ONE   (1 << FP_SHIFT)
#define FP_HALF  (1 << (FP_SHIFT - 1))
#define FP_PI    205887   /* π × 65536 */
#define FP_2PI   411775   /* 2π × 65536 */

/* Integer math */
uint32_t k_isqrt(uint32_t n);

/* Fixed-point operations */
int32_t k_sqrt_fp(int32_t x);
int32_t k_fixmul(int32_t a, int32_t b);
int32_t k_fixdiv(int32_t a, int32_t b);
int32_t k_sin_fp(int32_t x);
int32_t k_cos_fp(int32_t x);

/* Unqualified aliases */
int32_t fixmul(int32_t a, int32_t b);
int32_t fixdiv(int32_t a, int32_t b);
int32_t sin_fp(int32_t x);
int32_t cos_fp(int32_t x);

#endif /* _LIBKERNEL_MATH_H */
