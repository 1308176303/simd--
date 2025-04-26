#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "md5.h"
#include "md5_avx2.h"

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
    cout << "并行宽度: 8 (AVX2指令集8路并行版本)" << endl;
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
    double total_avx_time = 0;
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
        bit32** avx_results = new bit32*[DATA_SIZE];
        
        for (int i = 0; i < DATA_SIZE; i++) {
            serial_results[i] = new bit32[4];
            avx_results[i] = new bit32[4];
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
        
        // 执行AVX2 8路并行MD5测试
        cout << "执行AVX2 8路并行版本..." << flush;
        
        // 准备输入参数
        string* pw_array = new string[DATA_SIZE];
        for (int i = 0; i < DATA_SIZE; i++) {
            pw_array[i] = string(passwords[i]);
        }
        
        auto start_avx = high_resolution_clock::now();
        
        // 按批次调用AVX2版本 (每批8个)
        for (int i = 0; i < DATA_SIZE; i += 8) {
            int batch_size = (i + 8 <= DATA_SIZE) ? 8 : (DATA_SIZE - i);
            
            // 如果最后一批不足8个，需要补充到8个
            if (batch_size < 8) {
                for (int j = batch_size; j < 8; j++) {
                    pw_array[i + j] = pw_array[i]; // 用重复数据填充
                }
                batch_size = 8;
            }
            
            bit32* batch_results = new bit32[batch_size * 4];
            
            MD5Hash_AVX2(pw_array + i, batch_size, batch_results);
            
            // 拷贝结果（实际性能测试可以省略这一步，但为完整性保留）
            int valid_results = min(8, DATA_SIZE - i);
            for (int j = 0; j < valid_results; j++) {
                memcpy(avx_results[i + j], batch_results + j * 4, 4 * sizeof(bit32));
            }
            
            delete[] batch_results;
        }
        
        auto end_avx = high_resolution_clock::now();
        auto duration_avx = duration_cast<microseconds>(end_avx - start_avx).count();
        double time_avx = double(duration_avx) / 1000000.0;
        total_avx_time += time_avx;
        cout << " 完成" << endl;
        
        // 计算加速比
        double speedup = time_serial / time_avx;
        speedups[round] = speedup;
        
        // 输出性能结果
        cout << "串行版本耗时:  " << fixed << setprecision(4) << time_serial << " 秒" << endl;
        cout << "AVX2版本耗时:  " << fixed << setprecision(4) << time_avx << " 秒" << endl;
        cout << "加速比:        " << fixed << setprecision(2) << speedup << "x" << endl;
        cout << "吞吐量:        " << fixed << setprecision(0) << DATA_SIZE / time_avx << " 哈希/秒" << endl;
        cout << "------------------------------------------------------" << endl;
        
        // 释放内存
        for (int i = 0; i < DATA_SIZE; i++) {
            delete[] passwords[i];
            delete[] serial_results[i];
            delete[] avx_results[i];
        }
        
        delete[] passwords;
        delete[] serial_results;
        delete[] avx_results;
        delete[] pw_array;
    }
    
    // 计算平均性能
    double avg_serial_time = total_serial_time / TEST_ROUNDS;
    double avg_avx_time = total_avx_time / TEST_ROUNDS;
    double avg_speedup = avg_serial_time / avg_avx_time;
    
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
    cout << "平均AVX2时间:  " << fixed << setprecision(4) << avg_avx_time << " 秒/轮" << endl;
    cout << "平均加速比:    " << fixed << setprecision(2) << avg_speedup << "x" << endl;
    cout << "最小加速比:    " << fixed << setprecision(2) << min_speedup << "x" << endl;
    cout << "最大加速比:    " << fixed << setprecision(2) << max_speedup << "x" << endl;
    cout << "平均吞吐量:    " << fixed << setprecision(0) << DATA_SIZE / avg_avx_time << " 哈希/秒" << endl;
    cout << "=======================================================" << endl;
    
    return 0;
}