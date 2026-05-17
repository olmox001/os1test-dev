/*
 * kernel/libkernel/math.c
 * Fixed-point math functions for libkernel
 */
#include <libkernel/math.h>

/* These are also defined in libkernel/math.h — guards prevent redefinition errors */
#ifndef FP_SHIFT
#define FP_SHIFT 16
#endif
#ifndef FP_ONE
#define FP_ONE   (1 << FP_SHIFT)
#endif
#ifndef FP_HALF
#define FP_HALF  (1 << (FP_SHIFT - 1))
#endif
#ifndef FP_PI
#define FP_PI    205887
#endif
#ifndef FP_2PI
#define FP_2PI   411775
#endif

uint32_t k_isqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = (x + 1) >> 1;
    while (y < x) {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}

int32_t k_sqrt_fp(int32_t x) {
    if (x <= 0) return 0;
    uint64_t n = (uint64_t)x << 16;
    uint64_t root = k_isqrt((uint32_t)(n >> 16));
    return (int32_t)(root << 8);
}

int32_t k_fixmul(int32_t a, int32_t b) {
    int64_t result = (int64_t)a * b;
    return (int32_t)(result >> FP_SHIFT);
}

int32_t k_fixdiv(int32_t a, int32_t b) {
    if (b == 0) return 0;
    int64_t result = ((int64_t)a << FP_SHIFT) / b;
    return (int32_t)result;
}

int32_t k_sin_fp(int32_t x) {
    while (x > FP_PI) x -= FP_2PI;
    while (x < -FP_PI) x += FP_2PI;
    int32_t half_pi = 102944;
    if (x > half_pi) x = FP_PI - x;
    else if (x < -half_pi) x = -FP_PI - x;
    int32_t x2 = k_fixmul(x, x);
    int32_t x3 = k_fixmul(x2, x);
    int32_t x5 = k_fixmul(x3, x2);
    int32_t term1 = x;
    int32_t term2 = k_fixmul(x3, 10923);
    int32_t term3 = k_fixmul(x5, 546);
    return term1 - term2 + term3;
}

int32_t k_cos_fp(int32_t x) {
    return k_sin_fp(x + 102944);
}
/* User-space aliases */
int32_t fixmul(int32_t a, int32_t b) { return k_fixmul(a, b); }
int32_t fixdiv(int32_t a, int32_t b) { return k_fixdiv(a, b); }
int32_t sin_fp(int32_t x) { return k_sin_fp(x); }
int32_t cos_fp(int32_t x) { return k_cos_fp(x); }
