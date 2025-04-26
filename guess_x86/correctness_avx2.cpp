#include <chrono>
#include <iomanip>
#include "md5.h"
#include "md5_avx2.h"

using namespace std;
using namespace chrono;

// 打印MD5哈希值
void printHash(const bit32* state) {
    for(int i = 0; i < 4; i++) {
        cout << hex << setw(8) << setfill('0') << state[i];
    }
    cout << endl;
}

// 比较两个哈希值是否相同
bool compareHash(const bit32* hash1, const bit32* hash2) {
    for(int i = 0; i < 4; i++) {
        if(hash1[i] != hash2[i]) return false;
    }
    return true;
}

int main() {
    // 设置控制台编码，解决中文显示问题
    system("chcp 65001");
    
    // 测试串行版本
    string testString = "bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva";
    bit32 state[4];
    MD5Hash(testString, state);

    cout << "串行MD5结果: ";
    printHash(state);
    cout << endl;

    // 测试AVX2 8路并行版本
    cout << "测试AVX2 8路并行版本:" << endl;

    // 准备8个测试字符串
    string testStrings[8] = {
        "bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva", // 1
        "qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm",    // 2
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst",     // 3
        "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789",    // 4
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRST",     // 5
        "password1password1password1password1password1password1password1password1password1password1password1",     // 6
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",     // 7
        "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"      // 8
    };
    
    // 使用串行版本计算每个结果
    bit32 serialResults[8][4];  // 8个字符串，每个有4个32位整数的MD5结果
    for(int i = 0; i < 8; i++) {
        MD5Hash(testStrings[i], serialResults[i]);
        cout << "串行结果 [" << i << "]: ";
        printHash(serialResults[i]);
    }
    
    cout << endl;
    
    // 使用AVX2版本计算结果（8个一批）
    bit32 avxResults[8][4];
    
    // 一次处理所有8个字符串
    MD5Hash_AVX2(testStrings, 8, (bit32*)avxResults);
    
    // 比较结果
    cout << "比较结果:" << endl;
    bool allCorrect = true;
    
    for(int i = 0; i < 8; i++) {
        cout << "AVX2结果 [" << i << "]: ";
        printHash(avxResults[i]);
        
        if(!compareHash(serialResults[i], avxResults[i])) {
            cout << "错误: 通道 " << i << " 结果不匹配!" << endl;
            allCorrect = false;
        }
    }
    
    cout << endl;
    
    // 性能测试
    const int testCount = 10000;
    string testInput = "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789";
    
    // 串行版本
    auto start_serial = high_resolution_clock::now();
    for(int i = 0; i < testCount; i++) {
        MD5Hash(testInput, state);
    }
    auto end_serial = high_resolution_clock::now();
    auto duration_serial = duration_cast<microseconds>(end_serial - start_serial).count();
    
    // AVX2并行版本（8路）
    string* inputs = new string[testCount];
    for(int i = 0; i < testCount; i++) {
        inputs[i] = testInput;
    }
    
    bit32* results = new bit32[testCount * 4];
    
    auto start_avx = high_resolution_clock::now();
    for(int i = 0; i < testCount; i += 8) {
        int batchSize = min(8, testCount - i);
        // 如果不是8的倍数，需要补齐到8个
        if (batchSize < 8) {
            for (int j = batchSize; j < 8; j++) {
                inputs[i + j] = inputs[i]; // 用重复数据填充
            }
            batchSize = 8;
        }
        MD5Hash_AVX2(inputs + i, batchSize, results + i*4);
    }
    auto end_avx = high_resolution_clock::now();
    auto duration_avx = duration_cast<microseconds>(end_avx - start_avx).count();
    
    cout << "性能测试:" << endl;
    cout << "串行版本耗时: " << duration_serial << " 微秒" << endl;
    cout << "AVX2版本耗时: " << duration_avx << " 微秒" << endl;
    cout << "加速比: " << double(duration_serial) / duration_avx << "x" << endl;
    
    cout << endl;
    cout << "测试" << (allCorrect ? "通过!" : "失败!") << endl;
    
    delete[] inputs;
    delete[] results;
    
    return 0;
}