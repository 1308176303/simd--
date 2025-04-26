#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5.h"
#include <iomanip>
using namespace std;
using namespace chrono;

// 编译指令如下：
// g++ correctness.cpp train.cpp guessing.cpp md5.cpp -o test.exe


// 通过这个函数，你可以验证你实现的SIMD哈希函数的正确性
int main()
{
    // 测试串行版本
    bit32 state[4];
    string testString = "bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva";
    MD5Hash(testString, state);
    
    cout << "串行MD5结果: ";
    for (int i1 = 0; i1 < 4; i1 += 1)
    {
        cout << std::setw(8) << std::setfill('0') << hex << state[i1];
    }
    cout << endl;
    
    // 测试SIMD并行版本
    cout << "测试SIMD并行版本:" << endl;

    // 准备输入数据 - 每路输入不同以验证边界
    const int test_count = 4;    // 要测试的通道数
    string inputs[test_count];
    for (int i = 0; i < test_count; i++) {
        // 在原始 testString 后拼接索引，保证每个通道的数据不同
        inputs[i] = testString + "_" + to_string(i);
    }

    // 串行版本先逐个算出各通道的参考结果
    bit32 serial_states[test_count][4];
    for (int i = 0; i < test_count; i++) {
        MD5Hash(inputs[i], serial_states[i]);
    }

    // 准备输出存储空间
    bit32* simd_states[test_count];
    for (int i = 0; i < test_count; i++) {
        simd_states[i] = new bit32[4];
    }

    // 调用SIMD版本
    MD5Hash_SIMD(inputs, test_count, simd_states);

    // 输出并比较结果
    for (int i = 0; i < test_count; i++) {
        cout << "SIMD结果 " << i+1 << ": ";
        for (int j = 0; j < 4; j++) {
            cout << setw(8) << setfill('0') << hex << simd_states[i][j];
        }
        // 串行对照
        cout << "  串行结果: ";
        for (int j = 0; j < 4; j++) {
            cout << setw(8) << setfill('0') << hex << serial_states[i][j];
        }
        // 检查一致性
        bool match = true;
        for (int j = 0; j < 4; j++) {
            if (simd_states[i][j] != serial_states[i][j]) {
                match = false;
                break;
            }
        }
        cout << (match ? "  (匹配)" : "  (不匹配!)") << endl;
    }

    // 释放动态分配的内存
    for (int i = 0; i < test_count; i++) {
        delete[] simd_states[i];
    }
    
    return 0;
}