#include "md5_avx2.h"
#include "md5.h" // 引入原始MD5函数以复用StringProcess
#include <assert.h>
#include <cstring>

// 常量定义
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

// AVX2版本的MD5基本函数
#define F_AVX2(x, y, z) _mm256_or_si256(_mm256_and_si256((x), (y)), _mm256_andnot_si256((x), (z)))
#define G_AVX2(x, y, z) _mm256_or_si256(_mm256_and_si256((z), (x)), _mm256_andnot_si256((z), (y)))
#define H_AVX2(x, y, z) _mm256_xor_si256(_mm256_xor_si256((x), (y)), (z))
#define I_AVX2(x, y, z) _mm256_xor_si256((y), _mm256_or_si256((x), _mm256_xor_si256(_mm256_set1_epi32(0xffffffffU), (z))))

// AVX2位旋转
#define ROTATELEFT_AVX2(x, n) _mm256_or_si256(_mm256_slli_epi32((x), (n)), _mm256_srli_epi32((x), (32-(n))))

// AVX2版本的MD5转换
#define FF_AVX2(a, b, c, d, x, s, ac) { \
    (a) = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32((a), F_AVX2((b), (c), (d))), (x)), _mm256_set1_epi32(ac)); \
    (a) = ROTATELEFT_AVX2((a), (s)); \
    (a) = _mm256_add_epi32((a), (b)); \
    }

#define GG_AVX2(a, b, c, d, x, s, ac) { \
    (a) = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32((a), G_AVX2((b), (c), (d))), (x)), _mm256_set1_epi32(ac)); \
    (a) = ROTATELEFT_AVX2((a), (s)); \
    (a) = _mm256_add_epi32((a), (b)); \
    }

#define HH_AVX2(a, b, c, d, x, s, ac) { \
    (a) = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32((a), H_AVX2((b), (c), (d))), (x)), _mm256_set1_epi32(ac)); \
    (a) = ROTATELEFT_AVX2((a), (s)); \
    (a) = _mm256_add_epi32((a), (b)); \
    }

#define II_AVX2(a, b, c, d, x, s, ac) { \
    (a) = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32((a), I_AVX2((b), (c), (d))), (x)), _mm256_set1_epi32(ac)); \
    (a) = ROTATELEFT_AVX2((a), (s)); \
    (a) = _mm256_add_epi32((a), (b)); \
    }

/**
 * MD5Hash_AVX2: AVX2 8路并行版本的MD5实现
 * @param inputs 输入字符串数组
 * @param count 输入字符串的总数，必须是8的倍数
 * @param states 输出缓冲区，大小必须是count*4
 */
void MD5Hash_AVX2(const string* inputs, int count, bit32* states) {
    // 确保输入数量是8的倍数
    assert(count % 8 == 0);

    // 预处理所有输入字符串
    Byte** paddedMessages = new Byte*[count];
    int* messageLengths = new int[count];

    // 对每个输入字符串进行预处理
    for (int i = 0; i < count; i++) {
        paddedMessages[i] = StringProcess(inputs[i], &messageLengths[i]);
    }

    // 按8个一组进行处理
    for (int batch = 0; batch < count; batch += 8) {
        // 初始化状态向量
        __m256i a = _mm256_set1_epi32(0x67452301);
        __m256i b = _mm256_set1_epi32(0xefcdab89);
        __m256i c = _mm256_set1_epi32(0x98badcfe);
        __m256i d = _mm256_set1_epi32(0x10325476);
        
        // 验证8个消息长度相同
        for (int i = 1; i < 8; i++) {
            assert(messageLengths[batch] == messageLengths[batch + i]);
        }
        
        int n_blocks = messageLengths[batch] / 64;

        // 处理每个block
        for (int i = 0; i < n_blocks; i++) {
            __m256i x[16];

            // 将8个输入的相同位置字节加载到AVX2寄存器中
            for (int j = 0; j < 16; j++) {
                bit32 values[8];
                
                for (int k = 0; k < 8; k++) {
                    int msgIdx = batch + k;
                    values[k] = (paddedMessages[msgIdx][4*j + i*64]) |
                                (paddedMessages[msgIdx][4*j + 1 + i*64] << 8) |
                                (paddedMessages[msgIdx][4*j + 2 + i*64] << 16) |
                                (paddedMessages[msgIdx][4*j + 3 + i*64] << 24);
                }
                
                // 加载到AVX2寄存器
                x[j] = _mm256_set_epi32(values[7], values[6], values[5], values[4], 
                                        values[3], values[2], values[1], values[0]);
            }

            // 保存当前状态
            __m256i aa = a, bb = b, cc = c, dd = d;

            /* Round 1 */
            FF_AVX2(a, b, c, d, x[0], S11, 0xd76aa478);
            FF_AVX2(d, a, b, c, x[1], S12, 0xe8c7b756);
            FF_AVX2(c, d, a, b, x[2], S13, 0x242070db);
            FF_AVX2(b, c, d, a, x[3], S14, 0xc1bdceee);
            FF_AVX2(a, b, c, d, x[4], S11, 0xf57c0faf);
            FF_AVX2(d, a, b, c, x[5], S12, 0x4787c62a);
            FF_AVX2(c, d, a, b, x[6], S13, 0xa8304613);
            FF_AVX2(b, c, d, a, x[7], S14, 0xfd469501);
            FF_AVX2(a, b, c, d, x[8], S11, 0x698098d8);
            FF_AVX2(d, a, b, c, x[9], S12, 0x8b44f7af);
            FF_AVX2(c, d, a, b, x[10], S13, 0xffff5bb1);
            FF_AVX2(b, c, d, a, x[11], S14, 0x895cd7be);
            FF_AVX2(a, b, c, d, x[12], S11, 0x6b901122);
            FF_AVX2(d, a, b, c, x[13], S12, 0xfd987193);
            FF_AVX2(c, d, a, b, x[14], S13, 0xa679438e);
            FF_AVX2(b, c, d, a, x[15], S14, 0x49b40821);

            /* Round 2 */
            GG_AVX2(a, b, c, d, x[1], S21, 0xf61e2562);
            GG_AVX2(d, a, b, c, x[6], S22, 0xc040b340);
            GG_AVX2(c, d, a, b, x[11], S23, 0x265e5a51);
            GG_AVX2(b, c, d, a, x[0], S24, 0xe9b6c7aa);
            GG_AVX2(a, b, c, d, x[5], S21, 0xd62f105d);
            GG_AVX2(d, a, b, c, x[10], S22, 0x2441453);
            GG_AVX2(c, d, a, b, x[15], S23, 0xd8a1e681);
            GG_AVX2(b, c, d, a, x[4], S24, 0xe7d3fbc8);
            GG_AVX2(a, b, c, d, x[9], S21, 0x21e1cde6);
            GG_AVX2(d, a, b, c, x[14], S22, 0xc33707d6);
            GG_AVX2(c, d, a, b, x[3], S23, 0xf4d50d87);
            GG_AVX2(b, c, d, a, x[8], S24, 0x455a14ed);
            GG_AVX2(a, b, c, d, x[13], S21, 0xa9e3e905);
            GG_AVX2(d, a, b, c, x[2], S22, 0xfcefa3f8);
            GG_AVX2(c, d, a, b, x[7], S23, 0x676f02d9);
            GG_AVX2(b, c, d, a, x[12], S24, 0x8d2a4c8a);

            /* Round 3 */
            HH_AVX2(a, b, c, d, x[5], S31, 0xfffa3942);
            HH_AVX2(d, a, b, c, x[8], S32, 0x8771f681);
            HH_AVX2(c, d, a, b, x[11], S33, 0x6d9d6122);
            HH_AVX2(b, c, d, a, x[14], S34, 0xfde5380c);
            HH_AVX2(a, b, c, d, x[1], S31, 0xa4beea44);
            HH_AVX2(d, a, b, c, x[4], S32, 0x4bdecfa9);
            HH_AVX2(c, d, a, b, x[7], S33, 0xf6bb4b60);
            HH_AVX2(b, c, d, a, x[10], S34, 0xbebfbc70);
            HH_AVX2(a, b, c, d, x[13], S31, 0x289b7ec6);
            HH_AVX2(d, a, b, c, x[0], S32, 0xeaa127fa);
            HH_AVX2(c, d, a, b, x[3], S33, 0xd4ef3085);
            HH_AVX2(b, c, d, a, x[6], S34, 0x4881d05);
            HH_AVX2(a, b, c, d, x[9], S31, 0xd9d4d039);
            HH_AVX2(d, a, b, c, x[12], S32, 0xe6db99e5);
            HH_AVX2(c, d, a, b, x[15], S33, 0x1fa27cf8);
            HH_AVX2(b, c, d, a, x[2], S34, 0xc4ac5665);

            /* Round 4 */
            II_AVX2(a, b, c, d, x[0], S41, 0xf4292244);
            II_AVX2(d, a, b, c, x[7], S42, 0x432aff97);
            II_AVX2(c, d, a, b, x[14], S43, 0xab9423a7);
            II_AVX2(b, c, d, a, x[5], S44, 0xfc93a039);
            II_AVX2(a, b, c, d, x[12], S41, 0x655b59c3);
            II_AVX2(d, a, b, c, x[3], S42, 0x8f0ccc92);
            II_AVX2(c, d, a, b, x[10], S43, 0xffeff47d);
            II_AVX2(b, c, d, a, x[1], S44, 0x85845dd1);
            II_AVX2(a, b, c, d, x[8], S41, 0x6fa87e4f);
            II_AVX2(d, a, b, c, x[15], S42, 0xfe2ce6e0);
            II_AVX2(c, d, a, b, x[6], S43, 0xa3014314);
            II_AVX2(b, c, d, a, x[13], S44, 0x4e0811a1);
            II_AVX2(a, b, c, d, x[4], S41, 0xf7537e82);
            II_AVX2(d, a, b, c, x[11], S42, 0xbd3af235);
            II_AVX2(c, d, a, b, x[2], S43, 0x2ad7d2bb);
            II_AVX2(b, c, d, a, x[9], S44, 0xeb86d391);

            // 更新状态
            a = _mm256_add_epi32(a, aa);
            b = _mm256_add_epi32(b, bb);
            c = _mm256_add_epi32(c, cc);
            d = _mm256_add_epi32(d, dd);
        }

        // 存储结果到states数组
        // 注意：需要字节序调整
        alignas(32) bit32 state_a[8];
        alignas(32) bit32 state_b[8];
        alignas(32) bit32 state_c[8];
        alignas(32) bit32 state_d[8];
        
        _mm256_store_si256((__m256i*)state_a, a);
        _mm256_store_si256((__m256i*)state_b, b);
        _mm256_store_si256((__m256i*)state_c, c);
        _mm256_store_si256((__m256i*)state_d, d);
        
        // 处理所有通道
        for (int k = 0; k < 8; k++) {
            int outputIdx = batch + k;
            
            // 字节序调整
            bit32 values[4] = {state_a[k], state_b[k], state_c[k], state_d[k]};
            for (int i = 0; i < 4; i++) {
                bit32 value = values[i];
                states[outputIdx*4 + i] = ((value & 0xff) << 24) |
                                          ((value & 0xff00) << 8) |
                                          ((value & 0xff0000) >> 8) |
                                          ((value & 0xff000000) >> 24);
            }
        }
    }
    
    // 释放内存
    for (int i = 0; i < count; i++) {
        delete[] paddedMessages[i];
    }
    delete[] paddedMessages;
    delete[] messageLengths;
}