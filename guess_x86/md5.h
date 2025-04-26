#include <iostream>
#include <string>
#include <cstring>
#include <emmintrin.h>  // SSE2 指令集
#include <tmmintrin.h>  // SSSE3 指令集
#ifndef SIMD_WIDTH
#define SIMD_WIDTH 4  // SSE指令集自然并行宽度为4
#endif

using namespace std;

// 定义了Byte，便于使用
typedef unsigned char Byte;
// 定义了32比特
typedef unsigned int bit32;

// MD5的一系列参数。参数是固定的，其实你不需要看懂这些
#define s11 7
#define s12 12
#define s13 17
#define s14 22
#define s21 5
#define s22 9
#define s23 14
#define s24 20
#define s31 4
#define s32 11
#define s33 16
#define s34 23
#define s41 6
#define s42 10
#define s43 15
#define s44 21
Byte *StringProcess(string input, int *n_byte);
/**
 * @Basic MD5 functions.
 *
 * @param there bit32.
 *
 * @return one bit32.
 */
// 定义了一系列MD5中的具体函数
// 这四个计算函数是需要你进行SIMD并行化的
// 可以看到，FGHI四个函数都涉及一系列位运算，在数据上是对齐的，非常容易实现SIMD的并行化

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/**
 * @Rotate Left.
 *
 * @param {num} the raw number.
 *
 * @param {n} rotate left n.
 *
 * @return the number after rotated left.
 */
// 定义了一系列MD5中的具体函数
// 这五个计算函数（ROTATELEFT/FF/GG/HH/II）和之前的FGHI一样，都是需要你进行SIMD并行化的
// 但是你需要注意的是#define的功能及其效果，可以发现这里的FGHI是没有返回值的，为什么呢？你可以查询#define的含义和用法
#define ROTATELEFT(num, n) (((num) << (n)) | ((num) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
  (a) += F ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
  (a) += G ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
  (a) += H ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
  (a) += I ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}

void MD5Hash(string input, bit32 *state);
// ...existing code...



// SSE 版本的基础函数,适用于 AMD CPU
#define F_SSE(x, y, z) _mm_or_si128(_mm_and_si128((x), (y)), _mm_andnot_si128((x), (z))) 
#define G_SSE(x, y, z) _mm_or_si128(_mm_and_si128((x), (z)), _mm_and_si128((y), _mm_andnot_si128((z), _mm_set1_epi32(-1))))
#define H_SSE(x, y, z) _mm_xor_si128(_mm_xor_si128((x), (y)), (z))
#define I_SSE(x, y, z) _mm_xor_si128((y), _mm_or_si128((x), _mm_andnot_si128((z), _mm_set1_epi32(-1))))

// SSE 版本的左旋转
#define ROTATELEFT_SSE(x, n) \
    _mm_or_si128(_mm_slli_epi32((x), (n)), _mm_srli_epi32((x), (32-(n))))

// SSE 版本的轮函数
#define FF_SSE(a, b, c, d, x, s, ac) { \
    __m128i tmp = F_SSE((b), (c), (d)); \
    tmp = _mm_add_epi32(tmp, (x)); \
    tmp = _mm_add_epi32(tmp, _mm_set1_epi32(ac)); \
    (a) = _mm_add_epi32((a), tmp); \
    (a) = ROTATELEFT_SSE((a), (s)); \
    (a) = _mm_add_epi32((a), (b)); \
}

#define GG_SSE(a, b, c, d, x, s, ac) { \
    __m128i tmp = G_SSE((b), (c), (d)); \
    tmp = _mm_add_epi32(tmp, (x)); \
    tmp = _mm_add_epi32(tmp, _mm_set1_epi32(ac)); \
    (a) = _mm_add_epi32((a), tmp); \
    (a) = ROTATELEFT_SSE((a), (s)); \
    (a) = _mm_add_epi32((a), (b)); \
}

#define HH_SSE(a, b, c, d, x, s, ac) { \
    __m128i tmp = H_SSE((b), (c), (d)); \
    tmp = _mm_add_epi32(tmp, (x)); \
    tmp = _mm_add_epi32(tmp, _mm_set1_epi32(ac)); \
    (a) = _mm_add_epi32((a), tmp); \
    (a) = ROTATELEFT_SSE((a), (s)); \
    (a) = _mm_add_epi32((a), (b)); \
}

#define II_SSE(a, b, c, d, x, s, ac) { \
    __m128i tmp = I_SSE((b), (c), (d)); \
    tmp = _mm_add_epi32(tmp, (x)); \
    tmp = _mm_add_epi32(tmp, _mm_set1_epi32(ac)); \
    (a) = _mm_add_epi32((a), tmp); \
    (a) = ROTATELEFT_SSE((a), (s)); \
    (a) = _mm_add_epi32((a), (b)); \
}
void MD5Hash_SSE(const string* inputs, int count, bit32* states);
/**
 * MD5Hash_SSE2: 2路并行版本的MD5实现
 * @param inputs 输入字符串数组
 * @param count 输入字符串的总数，必须是2的倍数
 * @param states 输出缓冲区，大小必须是count*4
 */
void MD5Hash_SSE2(const string* inputs, int count, bit32* states);