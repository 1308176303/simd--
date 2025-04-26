#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "md5.h"



using namespace std;
using namespace chrono;

// 生成长度与示例近似的随机字符串
void generateLongRandomString(char* str, int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[length] = '\0';
}

int main() {
    // 设置控制台编码
    system("chcp 65001");
    
    cout << "====================== MD5性能测试 ======================" << endl;
    cout << "每轮测试: 10万条密码" << endl;
    cout << "重复次数: 10轮" << endl;
    cout << "并行宽度: " << SIMD_WIDTH << " (SSE指令集自然并行宽度)" << endl;
    cout << "=========================================================" << endl << endl;
    
    // 初始化随机数生成器
    srand((unsigned int)time(NULL));
    
    // 设置测试参数
    const int DATA_SIZE = 100000;  // 每轮10万条数据
    const int TEST_ROUNDS = 10;    // 总共10轮
    const int BASE_LENGTH = 100;   // 基础字符串长度
    const int LENGTH_VARIATION = 20; // 长度变化范围
    
    // 存储所有轮次的性能数据
    double total_serial_time = 0;
    double total_sse_time = 0;
    double speedups[TEST_ROUNDS];
    
    // 进行多轮测试
    for (int round = 0; round < TEST_ROUNDS; round++) {
        cout << "轮次 " << round + 1 << " / " << TEST_ROUNDS << ":" << endl;
        cout << "生成测试数据..." << flush;
        
        // 为该轮创建随机长度
        int this_round_length = BASE_LENGTH + (rand() % LENGTH_VARIATION);
        
        // 分配内存给密码和结果
        char** passwords = new char*[DATA_SIZE];
        for (int i = 0; i < DATA_SIZE; i++) {
            passwords[i] = new char[this_round_length + 1];
            generateLongRandomString(passwords[i], this_round_length);
        }
        
        // 分配结果存储空间
        bit32** serial_results = new bit32*[DATA_SIZE];
        bit32** sse_results = new bit32*[DATA_SIZE];
        
        for (int i = 0; i < DATA_SIZE; i++) {
            serial_results[i] = new bit32[4];
            sse_results[i] = new bit32[4];
        }
        
        cout << " 完成" << endl;
        cout << "字符串平均长度: " << this_round_length << " 字节" << endl;
        
        // 执行串行MD5测试
        cout << "执行串行版本..." << flush;
        auto start_serial = high_resolution_clock::now();
        
        for (int i = 0; i < DATA_SIZE; i++) {
            string pw(passwords[i]);
            MD5Hash(pw, serial_results[i]);
        }
        
        auto end_serial = high_resolution_clock::now();
        auto duration_serial = duration_cast<microseconds>(end_serial - start_serial).count();
        double time_serial = double(duration_serial) / 1000000.0;
        total_serial_time += time_serial;
        cout << " 完成" << endl;
        
        // 执行SSE并行MD5测试
        cout << "执行SSE并行版本..." << flush;
        
        // 准备输入参数
        string* pw_array = new string[DATA_SIZE];
        for (int i = 0; i < DATA_SIZE; i++) {
            pw_array[i] = string(passwords[i]);
        }
        
        auto start_sse = high_resolution_clock::now();
        
        // 按批次调用SSE版本 (每批SIMD_WIDTH个)
        for (int i = 0; i < DATA_SIZE; i += SIMD_WIDTH) {
            int batch_size = (i + SIMD_WIDTH <= DATA_SIZE) ? SIMD_WIDTH : (DATA_SIZE - i);
            bit32* batch_results = new bit32[batch_size * 4];
            
            MD5Hash_SSE(pw_array + i, batch_size, batch_results);
            
            // 拷贝结果（实际性能测试可以省略这一步，但为完整性保留）
            for (int j = 0; j < batch_size; j++) {
                memcpy(sse_results[i + j], batch_results + j * 4, 4 * sizeof(bit32));
            }
            
            delete[] batch_results;
        }
        
        auto end_sse = high_resolution_clock::now();
        auto duration_sse = duration_cast<microseconds>(end_sse - start_sse).count();
        double time_sse = double(duration_sse) / 1000000.0;
        total_sse_time += time_sse;
        cout << " 完成" << endl;
        
        // 计算加速比
        double speedup = time_serial / time_sse;
        speedups[round] = speedup;
        
        // 输出性能结果
        cout << "串行版本耗时:  " << fixed << setprecision(4) << time_serial << " 秒" << endl;
        cout << "SSE版本耗时:   " << fixed << setprecision(4) << time_sse << " 秒" << endl;
        cout << "加速比:        " << fixed << setprecision(2) << speedup << "x" << endl;
        cout << "吞吐量:        " << fixed << setprecision(0) << DATA_SIZE / time_sse << " 哈希/秒" << endl;
        cout << "------------------------------------------------------" << endl;
        
        // 释放内存
        for (int i = 0; i < DATA_SIZE; i++) {
            delete[] passwords[i];
            delete[] serial_results[i];
            delete[] sse_results[i];
        }
        
        delete[] passwords;
        delete[] serial_results;
        delete[] sse_results;
        delete[] pw_array;
    }
    
    // 计算平均性能
    double avg_serial_time = total_serial_time / TEST_ROUNDS;
    double avg_sse_time = total_sse_time / TEST_ROUNDS;
    double avg_speedup = avg_serial_time / avg_sse_time;
    
    // 计算最大和最小加速比
    double min_speedup = speedups[0];
    double max_speedup = speedups[0];
    for (int i = 1; i < TEST_ROUNDS; i++) {
        if (speedups[i] < min_speedup) min_speedup = speedups[i];
        if (speedups[i] > max_speedup) max_speedup = speedups[i];
    }
    
    // 输出总结
    cout << "\n====================== 测试总结 ======================" << endl;
    cout << "总数据量:      " << TEST_ROUNDS * DATA_SIZE << " 个密码" << endl;
    cout << "平均串行时间:  " << fixed << setprecision(4) << avg_serial_time << " 秒/轮" << endl;
    cout << "平均SSE时间:   " << fixed << setprecision(4) << avg_sse_time << " 秒/轮" << endl;
    cout << "平均加速比:    " << fixed << setprecision(2) << avg_speedup << "x" << endl;
    cout << "最小加速比:    " << fixed << setprecision(2) << min_speedup << "x" << endl;
    cout << "最大加速比:    " << fixed << setprecision(2) << max_speedup << "x" << endl;
    cout << "平均吞吐量:    " << fixed << setprecision(0) << DATA_SIZE / avg_sse_time << " 哈希/秒" << endl;
    cout << "=======================================================" << endl;
    
    return 0;
}