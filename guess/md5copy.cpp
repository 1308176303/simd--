#include "md5.h"
#include <iomanip>
#include <assert.h>
#include <chrono>

using namespace std;
using namespace chrono;

/**
 * StringProcess: 将单个输入字符串转换成MD5计算所需的消息数组
 * @param input 输入
 * @param[out] n_byte 用于给调用者传递额外的返回值，即最终Byte数组的长度
 * @return Byte消息数组
 */
Byte *StringProcess(string input, int *n_byte)
{
	// 将输入的字符串转换为Byte为单位的数组
	Byte *blocks = (Byte *)input.c_str();
	int length = input.length();

	// 计算原始消息长度（以比特为单位）
	int bitLength = length * 8;

	// paddingBits: 原始消息需要的padding长度（以bit为单位）
	// 对于给定的消息，将其补齐至length%512==448为止
	// 需要注意的是，即便给定的消息满足length%512==448，也需要再pad 512bits
	int paddingBits = bitLength % 512;
	if (paddingBits > 448)
	{
		paddingBits = 512 - (paddingBits - 448);
	}
	else if (paddingBits < 448)
	{
		paddingBits = 448 - paddingBits;
	}
	else if (paddingBits == 448)
	{
		paddingBits = 512;
	}

	// 原始消息需要的padding长度（以Byte为单位）
	int paddingBytes = paddingBits / 8;
	// 创建最终的字节数组
	// length + paddingBytes + 8:
	// 1. length为原始消息的长度（bits）
	// 2. paddingBytes为原始消息需要的padding长度（Bytes）
	// 3. 在pad到length%512==448之后，需要额外附加64bits的原始消息长度，即8个bytes
	int paddedLength = length + paddingBytes + 8;
	Byte *paddedMessage = new Byte[paddedLength];

	// 复制原始消息
	memcpy(paddedMessage, blocks, length);

	// 添加填充字节。填充时，第一位为1，后面的所有位均为0。
	// 所以第一个byte是0x80
	paddedMessage[length] = 0x80;							 // 添加一个0x80字节
	memset(paddedMessage + length + 1, 0, paddingBytes - 1); // 填充0字节

	// 添加消息长度（64比特，小端格式）
	for (int i = 0; i < 8; ++i)
	{
		// 特别注意此处应当将bitLength转换为uint64_t
		// 这里的length是原始消息的长度
		paddedMessage[length + paddingBytes + i] = ((uint64_t)length * 8 >> (i * 8)) & 0xFF;
	}

	// 验证长度是否满足要求。此时长度应当是512bit的倍数
	int residual = 8 * paddedLength % 512;
	// assert(residual == 0);

	// 在填充+添加长度之后，消息被分为n_blocks个512bit的部分
	*n_byte = paddedLength;
	return paddedMessage;
}


/**
 * MD5Hash: 将单个输入字符串转换成MD5
 * @param input 输入
 * @param[out] state 用于给调用者传递额外的返回值，即最终的缓冲区，也就是MD5的结果
 * @return Byte消息数组
 */
void MD5Hash(string input, bit32 *state)
{

	Byte *paddedMessage;
	int *messageLength = new int[1];
	for (int i = 0; i < 1; i += 1)
	{
		paddedMessage = StringProcess(input, &messageLength[i]);
		// cout<<messageLength[i]<<endl;
		assert(messageLength[i] == messageLength[0]);
	}
	int n_blocks = messageLength[0] / 64;

	// bit32* state= new bit32[4];
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;

	// 逐block地更新state
	for (int i = 0; i < n_blocks; i += 1)
	{
		bit32 x[16];

		// 下面的处理，在理解上较为复杂
		for (int i1 = 0; i1 < 16; ++i1)
		{
			x[i1] = (paddedMessage[4 * i1 + i * 64]) |
					(paddedMessage[4 * i1 + 1 + i * 64] << 8) |
					(paddedMessage[4 * i1 + 2 + i * 64] << 16) |
					(paddedMessage[4 * i1 + 3 + i * 64] << 24);
		}

		bit32 a = state[0], b = state[1], c = state[2], d = state[3];

		auto start = system_clock::now();
		/* Round 1 */
		FF(a, b, c, d, x[0], s11, 0xd76aa478);
		FF(d, a, b, c, x[1], s12, 0xe8c7b756);
		FF(c, d, a, b, x[2], s13, 0x242070db);
		FF(b, c, d, a, x[3], s14, 0xc1bdceee);
		FF(a, b, c, d, x[4], s11, 0xf57c0faf);
		FF(d, a, b, c, x[5], s12, 0x4787c62a);
		FF(c, d, a, b, x[6], s13, 0xa8304613);
		FF(b, c, d, a, x[7], s14, 0xfd469501);
		FF(a, b, c, d, x[8], s11, 0x698098d8);
		FF(d, a, b, c, x[9], s12, 0x8b44f7af);
		FF(c, d, a, b, x[10], s13, 0xffff5bb1);
		FF(b, c, d, a, x[11], s14, 0x895cd7be);
		FF(a, b, c, d, x[12], s11, 0x6b901122);
		FF(d, a, b, c, x[13], s12, 0xfd987193);
		FF(c, d, a, b, x[14], s13, 0xa679438e);
		FF(b, c, d, a, x[15], s14, 0x49b40821);

		/* Round 2 */
		GG(a, b, c, d, x[1], s21, 0xf61e2562);
		GG(d, a, b, c, x[6], s22, 0xc040b340);
		GG(c, d, a, b, x[11], s23, 0x265e5a51);
		GG(b, c, d, a, x[0], s24, 0xe9b6c7aa);
		GG(a, b, c, d, x[5], s21, 0xd62f105d);
		GG(d, a, b, c, x[10], s22, 0x2441453);
		GG(c, d, a, b, x[15], s23, 0xd8a1e681);
		GG(b, c, d, a, x[4], s24, 0xe7d3fbc8);
		GG(a, b, c, d, x[9], s21, 0x21e1cde6);
		GG(d, a, b, c, x[14], s22, 0xc33707d6);
		GG(c, d, a, b, x[3], s23, 0xf4d50d87);
		GG(b, c, d, a, x[8], s24, 0x455a14ed);
		GG(a, b, c, d, x[13], s21, 0xa9e3e905);
		GG(d, a, b, c, x[2], s22, 0xfcefa3f8);
		GG(c, d, a, b, x[7], s23, 0x676f02d9);
		GG(b, c, d, a, x[12], s24, 0x8d2a4c8a);

		/* Round 3 */
		HH(a, b, c, d, x[5], s31, 0xfffa3942);
		HH(d, a, b, c, x[8], s32, 0x8771f681);
		HH(c, d, a, b, x[11], s33, 0x6d9d6122);
		HH(b, c, d, a, x[14], s34, 0xfde5380c);
		HH(a, b, c, d, x[1], s31, 0xa4beea44);
		HH(d, a, b, c, x[4], s32, 0x4bdecfa9);
		HH(c, d, a, b, x[7], s33, 0xf6bb4b60);
		HH(b, c, d, a, x[10], s34, 0xbebfbc70);
		HH(a, b, c, d, x[13], s31, 0x289b7ec6);
		HH(d, a, b, c, x[0], s32, 0xeaa127fa);
		HH(c, d, a, b, x[3], s33, 0xd4ef3085);
		HH(b, c, d, a, x[6], s34, 0x4881d05);
		HH(a, b, c, d, x[9], s31, 0xd9d4d039);
		HH(d, a, b, c, x[12], s32, 0xe6db99e5);
		HH(c, d, a, b, x[15], s33, 0x1fa27cf8);
		HH(b, c, d, a, x[2], s34, 0xc4ac5665);

		/* Round 4 */
		II(a, b, c, d, x[0], s41, 0xf4292244);
		II(d, a, b, c, x[7], s42, 0x432aff97);
		II(c, d, a, b, x[14], s43, 0xab9423a7);
		II(b, c, d, a, x[5], s44, 0xfc93a039);
		II(a, b, c, d, x[12], s41, 0x655b59c3);
		II(d, a, b, c, x[3], s42, 0x8f0ccc92);
		II(c, d, a, b, x[10], s43, 0xffeff47d);
		II(b, c, d, a, x[1], s44, 0x85845dd1);
		II(a, b, c, d, x[8], s41, 0x6fa87e4f);
		II(d, a, b, c, x[15], s42, 0xfe2ce6e0);
		II(c, d, a, b, x[6], s43, 0xa3014314);
		II(b, c, d, a, x[13], s44, 0x4e0811a1);
		II(a, b, c, d, x[4], s41, 0xf7537e82);
		II(d, a, b, c, x[11], s42, 0xbd3af235);
		II(c, d, a, b, x[2], s43, 0x2ad7d2bb);
		II(b, c, d, a, x[9], s44, 0xeb86d391);

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
	}

	// 下面的处理，在理解上较为复杂
	for (int i = 0; i < 4; i++)
	{
		uint32_t value = state[i];
		state[i] = ((value & 0xff) << 24) |		 // 将最低字节移到最高位
				   ((value & 0xff00) << 8) |	 // 将次低字节左移
				   ((value & 0xff0000) >> 8) |	 // 将次高字节右移
				   ((value & 0xff000000) >> 24); // 将最高字节移到最低位
	}

	// 输出最终的hash结果
	// for (int i1 = 0; i1 < 4; i1 += 1)
	// {
	// 	cout << std::setw(8) << std::setfill('0') << hex << state[i1];
	// }
	// cout << endl;

	// 释放动态分配的内存
	// 实现SIMD并行算法的时候，也请记得及时回收内存！
	delete[] paddedMessage;
	delete[] messageLength;
}



/**
 * StringProcess_SIMD: 优化的并行预处理函数
 */
void StringProcess_SIMD(const string* inputs, size_t input_count, Byte** paddedMessages, int* messageLengths)
{
    // 改为每次处理4个输入 (NEON自然宽度)
    #pragma omp parallel for if(input_count > 32)  // 只在处理大量消息时并行化
	const size_t MAX_MSG_LEN = 2048; // 假设大多数密码长度不超过此值
	const size_t ALIGNED_MSG_SIZE = (MAX_MSG_LEN + 15) & ~15; // 16字节对齐
    for (size_t i = 0; i < input_count; i++)
    {
        // 计算填充后长度
        const string& input = inputs[i];
        int length = input.length();
        int bitLength = length * 8;
        
        // 计算padding位数
        int paddingBits = bitLength % 512;
        if (paddingBits > 448) {
            paddingBits = 512 - (paddingBits - 448);
        } else if (paddingBits < 448) {
            paddingBits = 448 - paddingBits;
        } else {
            paddingBits = 512;
        }
        
        // 计算字节数
        int paddingBytes = paddingBits / 8;
        int paddedLength = length + paddingBytes + 8;
        messageLengths[i] = paddedLength;
        
        // 使用对齐内存分配
        void* ptr = nullptr;
        if (posix_memalign(&ptr, 16, paddedLength) != 0) {
            // 分配失败时回退到普通分配
            paddedMessages[i] = new Byte[paddedLength];
        } else {
            paddedMessages[i] = static_cast<Byte*>(ptr);
        }
        
        // 复制原始消息
        memcpy(paddedMessages[i], input.c_str(), length);
        
        // 添加填充
        paddedMessages[i][length] = 0x80;
        memset(paddedMessages[i] + length + 1, 0, paddingBytes - 1);
        
        // 添加消息长度
        for (int j = 0; j < 8; ++j) {
            paddedMessages[i][length + paddingBytes + j] = ((uint64_t)length * 8 >> (j * 8)) & 0xFF;
        }
    }
}

/**
 * MD5Hash_SIMD: 使用SIMD指令并行计算多个MD5哈希
 * @param inputs 多个输入字符串的数组
 * @param input_count 输入字符串数量
 * @param[out] states 用于存储多个哈希结果的数组
 */
void MD5Hash_SIMD(const string* inputs, size_t input_count, bit32** states)
{
    // 修正SIMD宽度
    const int SIMD_WIDTH = 8;  // NEON最优处理4个32位整数
    
    // 内存分配
    Byte** paddedMessages = new Byte*[input_count];
    int* messageLengths = new int[input_count];
    
    // 预处理所有输入
    StringProcess_SIMD(inputs, input_count, paddedMessages, messageLengths);
    
    // 按批次处理
    for (size_t base = 0; base < input_count; base += SIMD_WIDTH)
    {
        size_t current_batch = std::min(size_t(SIMD_WIDTH), input_count - base);
        
        // 找出最大块数
        int max_blocks = 0;
        for (size_t i = 0; i < current_batch; i++) {
            int blocks = messageLengths[base + i] / 64;
            if (blocks > max_blocks) max_blocks = blocks;
        }
        
        // 初始化状态
        uint32x4_t state0 = vdupq_n_u32(0x67452301);
        uint32x4_t state1 = vdupq_n_u32(0xefcdab89);
        uint32x4_t state2 = vdupq_n_u32(0x98badcfe);
        uint32x4_t state3 = vdupq_n_u32(0x10325476);
        
        // 批量处理块
        for (int block = 0; block < max_blocks; block++)
        {
			if (block + 1 < max_blocks) {
				for (size_t i = 0; i < current_batch; i++) {
					int blocks_in_message = messageLengths[base + i] / 64;
					if (block + 1 < blocks_in_message) {
						__builtin_prefetch(paddedMessages[base + i] + (block + 1) * 64, 0, 3);
					}
				}
			}
            // 优化数据加载，每批16个4元素向量
            uint32x4_t x[16];
            
            // 优化2: 批量加载多个32位块，减少循环次数
			for (int j = 0; j < 16; j += 4) {
				uint32_t values0[4] __attribute__((aligned(16))) = {0}; 
				uint32_t values1[4] __attribute__((aligned(16))) = {0};
				uint32_t values2[4] __attribute__((aligned(16))) = {0};
				uint32_t values3[4] __attribute__((aligned(16))) = {0};
				
				for (size_t i = 0; i < current_batch; i++) {
					int blocks_in_message = messageLengths[base + i] / 64;
					if (block < blocks_in_message) {
						
						// 计算起始地址一次，重复使用
						uint32_t* word_ptr = reinterpret_cast<uint32_t*>(
							paddedMessages[base + i] + block * 64 + j * 4);
						
						// 一次预取所有需要的数据，避免重复循环
						if (j + 4 < 16 && block < blocks_in_message) {
							__builtin_prefetch(word_ptr + 4, 0, 2);  // 直接预取下一批数据
						}
						// 简化后的访问 - 由于j的步长是4，最大j=12，所以j+1/2/3 < 16总是成立
						values0[i] = word_ptr[0];
						values1[i] = word_ptr[1];
						values2[i] = word_ptr[2];
						values3[i] = word_ptr[3];
					}
				}
				
				// 一次加载多个向量 - 无需检查j+1/2/3 < 16
				x[j] = vld1q_u32(values0);
				x[j+1] = vld1q_u32(values1);
				x[j+2] = vld1q_u32(values2);
				x[j+3] = vld1q_u32(values3);
			}
            
            // 保存状态
            uint32x4_t a = state0, b = state1, c = state2, d = state3;
            
            // 使用宏执行MD5四轮操作 (代码同原始版本)
            /* Round 1 */
			FF_SIMD(a, b, c, d, x[0], s11, 0xd76aa478);
			FF_SIMD(d, a, b, c, x[1], s12, 0xe8c7b756);
			FF_SIMD(c, d, a, b, x[2], s13, 0x242070db);
			FF_SIMD(b, c, d, a, x[3], s14, 0xc1bdceee);
			FF_SIMD(a, b, c, d, x[4], s11, 0xf57c0faf);
			FF_SIMD(d, a, b, c, x[5], s12, 0x4787c62a);
			FF_SIMD(c, d, a, b, x[6], s13, 0xa8304613);
			FF_SIMD(b, c, d, a, x[7], s14, 0xfd469501);
			FF_SIMD(a, b, c, d, x[8], s11, 0x698098d8);
			FF_SIMD(d, a, b, c, x[9], s12, 0x8b44f7af);
			FF_SIMD(c, d, a, b, x[10], s13, 0xffff5bb1);
			FF_SIMD(b, c, d, a, x[11], s14, 0x895cd7be);
			FF_SIMD(a, b, c, d, x[12], s11, 0x6b901122);
			FF_SIMD(d, a, b, c, x[13], s12, 0xfd987193);
			FF_SIMD(c, d, a, b, x[14], s13, 0xa679438e);
			FF_SIMD(b, c, d, a, x[15], s14, 0x49b40821);

			/* Round 2 */
			GG_SIMD(a, b, c, d, x[1], s21, 0xf61e2562);
			GG_SIMD(d, a, b, c, x[6], s22, 0xc040b340);
			GG_SIMD(c, d, a, b, x[11], s23, 0x265e5a51);
			GG_SIMD(b, c, d, a, x[0], s24, 0xe9b6c7aa);
			GG_SIMD(a, b, c, d, x[5], s21, 0xd62f105d);
			GG_SIMD(d, a, b, c, x[10], s22, 0x2441453);
			GG_SIMD(c, d, a, b, x[15], s23, 0xd8a1e681);
			GG_SIMD(b, c, d, a, x[4], s24, 0xe7d3fbc8);
			GG_SIMD(a, b, c, d, x[9], s21, 0x21e1cde6);
			GG_SIMD(d, a, b, c, x[14], s22, 0xc33707d6);
			GG_SIMD(c, d, a, b, x[3], s23, 0xf4d50d87);
			GG_SIMD(b, c, d, a, x[8], s24, 0x455a14ed);
			GG_SIMD(a, b, c, d, x[13], s21, 0xa9e3e905);
			GG_SIMD(d, a, b, c, x[2], s22, 0xfcefa3f8);
			GG_SIMD(c, d, a, b, x[7], s23, 0x676f02d9);
			GG_SIMD(b, c, d, a, x[12], s24, 0x8d2a4c8a);

			/* Round 3 */
			HH_SIMD(a, b, c, d, x[5], s31, 0xfffa3942);
			HH_SIMD(d, a, b, c, x[8], s32, 0x8771f681);
			HH_SIMD(c, d, a, b, x[11], s33, 0x6d9d6122);
			HH_SIMD(b, c, d, a, x[14], s34, 0xfde5380c);
			HH_SIMD(a, b, c, d, x[1], s31, 0xa4beea44);
			HH_SIMD(d, a, b, c, x[4], s32, 0x4bdecfa9);
			HH_SIMD(c, d, a, b, x[7], s33, 0xf6bb4b60);
			HH_SIMD(b, c, d, a, x[10], s34, 0xbebfbc70);
			HH_SIMD(a, b, c, d, x[13], s31, 0x289b7ec6);
			HH_SIMD(d, a, b, c, x[0], s32, 0xeaa127fa);
			HH_SIMD(c, d, a, b, x[3], s33, 0xd4ef3085);
			HH_SIMD(b, c, d, a, x[6], s34, 0x4881d05);
			HH_SIMD(a, b, c, d, x[9], s31, 0xd9d4d039);
			HH_SIMD(d, a, b, c, x[12], s32, 0xe6db99e5);
			HH_SIMD(c, d, a, b, x[15], s33, 0x1fa27cf8);
			HH_SIMD(b, c, d, a, x[2], s34, 0xc4ac5665);

			/* Round 4 */
			II_SIMD(a, b, c, d, x[0], s41, 0xf4292244);
			II_SIMD(d, a, b, c, x[7], s42, 0x432aff97);
			II_SIMD(c, d, a, b, x[14], s43, 0xab9423a7);
			II_SIMD(b, c, d, a, x[5], s44, 0xfc93a039);
			II_SIMD(a, b, c, d, x[12], s41, 0x655b59c3);
			II_SIMD(d, a, b, c, x[3], s42, 0x8f0ccc92);
			II_SIMD(c, d, a, b, x[10], s43, 0xffeff47d);
			II_SIMD(b, c, d, a, x[1], s44, 0x85845dd1);
			II_SIMD(a, b, c, d, x[8], s41, 0x6fa87e4f);
			II_SIMD(d, a, b, c, x[15], s42, 0xfe2ce6e0);
			II_SIMD(c, d, a, b, x[6], s43, 0xa3014314);
			II_SIMD(b, c, d, a, x[13], s44, 0x4e0811a1);
			II_SIMD(a, b, c, d, x[4], s41, 0xf7537e82);
			II_SIMD(d, a, b, c, x[11], s42, 0xbd3af235);
			II_SIMD(c, d, a, b, x[2], s43, 0x2ad7d2bb);
			II_SIMD(b, c, d, a, x[9], s44, 0xeb86d391);
            // ... (其他FF_SIMD, GG_SIMD, HH_SIMD, II_SIMD调用保持不变)
            
            // 更新状态
            state0 = vaddq_u32(state0, a);
            state1 = vaddq_u32(state1, b);
            state2 = vaddq_u32(state2, c);
            state3 = vaddq_u32(state3, d);
        }
        
        // 字节序翻转
        uint32x4_t final0 = ByteSwapSIMD(state0);
        uint32x4_t final1 = ByteSwapSIMD(state1);
        uint32x4_t final2 = ByteSwapSIMD(state2);
        uint32x4_t final3 = ByteSwapSIMD(state3);
        
        // 使用对齐存储
        uint32_t output[4][4] __attribute__((aligned(16)));
        vst1q_u32(output[0], final0);
        vst1q_u32(output[1], final1);
        vst1q_u32(output[2], final2);
        vst1q_u32(output[3], final3);
        
        // 重新排列结果
        for (size_t i = 0; i < current_batch; i++) {
            states[base + i][0] = output[0][i];
            states[base + i][1] = output[1][i];
            states[base + i][2] = output[2][i];
            states[base + i][3] = output[3][i];
        }
    }
    
    // 释放内存
    // 释放内存时，直接使用free，不需要检查对齐
	for (size_t i = 0; i < input_count; i++) {
		if (paddedMessages[i]) {
			free(paddedMessages[i]);  // 所有内存都是通过posix_memalign分配的
		}
	}
    delete[] paddedMessages;
    delete[] messageLengths;
}
