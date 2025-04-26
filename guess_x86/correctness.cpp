#include <chrono>
#include <iomanip>
#include "md5.h"

using namespace std;
using namespace chrono;

// 打印MD5哈希值
void printHash(const bit32* state) {
    for(int i = 0; i < 4; i++) {
        cout << std::setw(8) << std::setfill('0') << hex << state[i];
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

    // 测试SSE并行版本
    cout << "测试SSE并行版本:" << endl;

    // 准备4组相同长度的测试数据
    const int test_count = 4;  // SSE一次处理4个数据
    string inputs[test_count];
    for(int i = 0; i < test_count; i++) {
        inputs[i] = testString + to_string(i);  // 每组数据添加不同后缀
    }

    // 用串行版本计算参考结果
    bit32 serial_results[test_count][4];
    for(int i = 0; i < test_count; i++) {
        MD5Hash(inputs[i], serial_results[i]);
        cout << "串行结果 [" << i << "]: ";
        printHash(serial_results[i]);
    }

    // 使用SSE版本计算
    bit32 sse_results[test_count * 4];  // 存储SSE结果
    MD5Hash_SSE(inputs, test_count, sse_results);

    // 比较结果
    bool all_correct = true;
    cout << "\n比较结果:" << endl;
    for(int i = 0; i < test_count; i++) {
        cout << "SSE结果  [" << i << "]: ";
        printHash(sse_results + i * 4);
        
        if(!compareHash(serial_results[i], sse_results + i * 4)) {
            cout << "错误: 通道 " << i << " 结果不匹配!" << endl;
            all_correct = false;
        }
    }

    // 性能测试
    cout << "\n性能测试:" << endl;
    const int PERF_ITERATIONS = 1000;
    
    // 测试串行版本性能
    auto start = high_resolution_clock::now();
    for(int i = 0; i < PERF_ITERATIONS; i++) {
        MD5Hash(inputs[0], state);
    }
    auto end = high_resolution_clock::now();
    auto serial_duration = duration_cast<microseconds>(end - start).count();

    // 测试SSE版本性能
    start = high_resolution_clock::now();
    for(int i = 0; i < PERF_ITERATIONS / 4; i++) {
        MD5Hash_SSE(inputs, 4, sse_results);
    }
    end = high_resolution_clock::now();
    auto sse_duration = duration_cast<microseconds>(end - start).count();

    cout << "串行版本耗时: " << serial_duration << " 微秒" << endl;
    cout << "SSE版本耗时:  " << sse_duration << " 微秒" << endl;
    cout << "加速比: " << (float)serial_duration / sse_duration << "x" << endl;

    if(all_correct) {
        cout << "\n所有测试通过!" << endl;
        return 0;
    } else {
        cout << "\n测试失败!" << endl;
        return 1;
    }
}