#ifndef _LIBKERNEL_MATH_H
#define _LIBKERNEL_MATH_H

#include <libkernel/types.h>

uint32_t k_isqrt(uint32_t n);
int32_t k_sqrt_fp(int32_t x);
int32_t k_fixmul(int32_t a, int32_t b);
int32_t k_fixdiv(int32_t a, int32_t b);
int32_t k_sin_fp(int32_t x);
int32_t k_cos_fp(int32_t x);

int32_t fixmul(int32_t a, int32_t b);
int32_t fixdiv(int32_t a, int32_t b);
int32_t sin_fp(int32_t x);
int32_t cos_fp(int32_t x);

#endif
