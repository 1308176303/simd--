#include <chrono>
#include <iomanip>
#include "md5.h"

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

    // 测试SSE 2路并行版本
    cout << "测试SSE 2路并行版本:" << endl;

    // 准备4个测试字符串
    string testStrings[4] = {
        "bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva",
        "qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm",
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst",
        "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
    };
    
    // 使用串行版本计算每个结果
    bit32 serialResults[4][4];  // 4个字符串，每个有4个32位整数的MD5结果
    for(int i = 0; i < 4; i++) {
        MD5Hash(testStrings[i], serialResults[i]);
        cout << "串行结果 [" << i << "]: ";
        printHash(serialResults[i]);
    }
    
    cout << endl;
    
    // 使用SSE2版本计算结果（两个一批）
    bit32 sse2Results[4][4];
    
    // 第一批：前两个字符串
    MD5Hash_SSE2(testStrings, 2, (bit32*)sse2Results);
    
    // 第二批：后两个字符串
    MD5Hash_SSE2(testStrings + 2, 2, (bit32*)(sse2Results + 2));
    
    // 比较结果
    cout << "比较结果:" << endl;
    bool allCorrect = true;
    
    for(int i = 0; i < 4; i++) {
        cout << "SSE2结果 [" << i << "]: ";
        printHash(sse2Results[i]);
        
        if(!compareHash(serialResults[i], sse2Results[i])) {
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
    
    // SSE2并行版本（2路）
    string* inputs = new string[testCount];
    for(int i = 0; i < testCount; i++) {
        inputs[i] = testInput;
    }
    
    bit32* results = new bit32[testCount * 4];
    
    auto start_sse2 = high_resolution_clock::now();
    for(int i = 0; i < testCount; i += 2) {
        int batchSize = min(2, testCount - i);
        MD5Hash_SSE2(inputs + i, batchSize, results + i*4);
    }
    auto end_sse2 = high_resolution_clock::now();
    auto duration_sse2 = duration_cast<microseconds>(end_sse2 - start_sse2).count();
    
    cout << "性能测试:" << endl;
    cout << "串行版本耗时: " << duration_serial << " 微秒" << endl;
    cout << "SSE2版本耗时: " << duration_sse2 << " 微秒" << endl;
    cout << "加速比: " << double(duration_serial) / duration_sse2 << "x" << endl;
    
    cout << endl;
    cout << "测试" << (allCorrect ? "通过!" : "失败!") << endl;
    
    delete[] inputs;
    delete[] results;
    
    return 0;
}