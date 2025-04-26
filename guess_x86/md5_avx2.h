#ifndef MD5_AVX2_H
#define MD5_AVX2_H

#include <string>
#include <immintrin.h> // 包含AVX2指令集

using namespace std;

// 定义了Byte，便于使用
typedef unsigned char Byte;
// 定义了32比特
typedef unsigned int bit32;

/**
 * MD5Hash_AVX2: AVX2 8路并行版本的MD5实现
 * @param inputs 输入字符串数组
 * @param count 输入字符串的总数，必须是8的倍数
 * @param states 输出缓冲区，大小必须是count*4
 */
void MD5Hash_AVX2(const string* inputs, int count, bit32* states);

#endif // MD5_AVX2_H