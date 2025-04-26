#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>  // 用于写入CSV文件
#include <windows.h> // 添加Windows.h以使用Sleep函数
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
    
    // 添加系统稳定性保障 - 设置高性能电源计划
    system("powercfg /setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c");
    
    cout << "================= MD5哈希并行度对比测试 =================" << endl;
    cout << "CPU型号: AMD Ryzen 7 7840H" << endl;
    cout << "测试内容: 串行vs2路SSE2 vs4路SSE vs8路AVX2" << endl;
    cout << "=========================================================" << endl << endl;
    
    // 创建CSV文件
    ofstream csvFile("md5_performance_results.csv");
    if (!csvFile.is_open()) {
        cout << "无法创建CSV文件!" << endl;
        return 1;
    }
    
    // 写入CSV文件头
    csvFile << "数据量,密码长度,串行(秒),2路SSE2(秒),4路SSE(秒),8路AVX2(秒),2路加速比,4路加速比,8路加速比" << endl;
    
    // 初始化随机数生成器
    srand((unsigned int)time(NULL));
    
    // 设置测试参数 - 使用大规模数据
    const int DATA_SIZES_COUNT = 5;
    const int dataSizes[DATA_SIZES_COUNT] = {10000, 50000, 100000, 250000, 500000};
    
    const int PASSWORD_LENGTHS_COUNT = 5;
    const int passwordLengths[PASSWORD_LENGTHS_COUNT] = {16, 32, 64, 128, 256};
    
    // 增加测试轮次以稳定数据
    const int TEST_ROUNDS = 10;  // 从原来的3轮增加到10轮
    
    // 创建结果表格
    cout << "数据量\t密码长度\t串行(秒)\t2路SSE2(秒)\t4路SSE(秒)\t8路AVX2(秒)\t2路加速\t4路加速\t8路加速" << endl;
    
    // 对每种数据规模和密码长度组合进行测试
    for (int i = 0; i < DATA_SIZES_COUNT; i++) {
        int dataSize = dataSizes[i];
        
        for (int j = 0; j < PASSWORD_LENGTHS_COUNT; j++) {
            int pwLength = passwordLengths[j];
            
            // 对特定组合（小数据量）进行额外测试
            int actual_rounds = TEST_ROUNDS;
            if (dataSize <= 50000) {
                // 小数据量配置可能更不稳定，增加轮次
                actual_rounds = TEST_ROUNDS * 2;
            }
            
            // 存储所有版本的性能数据
            double total_serial_time = 0;
            double total_sse2_time = 0;
            double total_sse4_time = 0;
            double total_avx2_time = 0;
            
            for (int round = 0; round < actual_rounds; round++) {
                cout << "测试: " << dataSize << "个密码 x " << pwLength 
                     << "字符长度 (轮次 " << round+1 << "/" << actual_rounds << ")..." << flush;
                
                // 测试前暂停以等待系统稳定
                Sleep(1000);
                
                // 生成测试数据
                char** passwords = new char*[dataSize];
                for (int k = 0; k < dataSize; k++) {
                    passwords[k] = new char[pwLength + 1];
                    generateLongRandomString(passwords[k], pwLength);
                }
                
                // 准备输入参数
                string* pw_array = new string[dataSize];
                for (int k = 0; k < dataSize; k++) {
                    pw_array[k] = string(passwords[k]);
                }
                
                // 分配结果存储空间
                bit32** results = new bit32*[dataSize];
                for (int k = 0; k < dataSize; k++) {
                    results[k] = new bit32[4];
                }
                
                // 1. 测试串行版本
                auto start_serial = high_resolution_clock::now();
                
                for (int k = 0; k < dataSize; k++) {
                    MD5Hash(pw_array[k], results[k]);
                }
                
                auto end_serial = high_resolution_clock::now();
                auto duration_serial = duration_cast<microseconds>(end_serial - start_serial).count();
                double time_serial = double(duration_serial) / 1000000.0;
                total_serial_time += time_serial;
                
                // 2. 测试2路SSE2版本
                auto start_sse2 = high_resolution_clock::now();
                
                for (int k = 0; k < dataSize; k += 2) {
                    int batch_size = (k + 2 <= dataSize) ? 2 : (dataSize - k);
                    // 如果最后一批不足2个，需要补齐
                    if (batch_size < 2) {
                        pw_array[k+1] = pw_array[k];
                        batch_size = 2;
                    }
                    
                    bit32* batch_results = new bit32[batch_size * 4];
                    MD5Hash_SSE2(pw_array + k, batch_size, batch_results);
                    
                    delete[] batch_results;
                }
                
                auto end_sse2 = high_resolution_clock::now();
                auto duration_sse2 = duration_cast<microseconds>(end_sse2 - start_sse2).count();
                double time_sse2 = double(duration_sse2) / 1000000.0;
                total_sse2_time += time_sse2;
                
                // 3. 测试4路SSE版本
                auto start_sse4 = high_resolution_clock::now();
                
                for (int k = 0; k < dataSize; k += 4) {
                    int batch_size = (k + 4 <= dataSize) ? 4 : (dataSize - k);
                    // 如果最后一批不足4个，需要补齐
                    if (batch_size < 4) {
                        for (int m = batch_size; m < 4; m++) {
                            pw_array[k + m] = pw_array[k];
                        }
                        batch_size = 4;
                    }
                    
                    bit32* batch_results = new bit32[batch_size * 4];
                    MD5Hash_SSE(pw_array + k, batch_size, batch_results);
                    
                    delete[] batch_results;
                }
                
                auto end_sse4 = high_resolution_clock::now();
                auto duration_sse4 = duration_cast<microseconds>(end_sse4 - start_sse4).count();
                double time_sse4 = double(duration_sse4) / 1000000.0;
                total_sse4_time += time_sse4;
                
                // 4. 测试8路AVX2版本
                auto start_avx2 = high_resolution_clock::now();
                
                for (int k = 0; k < dataSize; k += 8) {
                    int batch_size = (k + 8 <= dataSize) ? 8 : (dataSize - k);
                    // 如果最后一批不足8个，需要补齐
                    if (batch_size < 8) {
                        for (int m = batch_size; m < 8; m++) {
                            pw_array[k + m] = pw_array[k];
                        }
                        batch_size = 8;
                    }
                    
                    bit32* batch_results = new bit32[batch_size * 4];
                    MD5Hash_AVX2(pw_array + k, batch_size, batch_results);
                    
                    delete[] batch_results;
                }
                
                auto end_avx2 = high_resolution_clock::now();
                auto duration_avx2 = duration_cast<microseconds>(end_avx2 - start_avx2).count();
                double time_avx2 = double(duration_avx2) / 1000000.0;
                total_avx2_time += time_avx2;
                
                cout << "完成" << endl;
                
                // 释放内存
                for (int k = 0; k < dataSize; k++) {
                    delete[] passwords[k];
                    delete[] results[k];
                }
                
                delete[] passwords;
                delete[] pw_array;
                delete[] results;
            }
            
            // 计算平均性能
            double avg_serial = total_serial_time / actual_rounds;
            double avg_sse2 = total_sse2_time / actual_rounds;
            double avg_sse4 = total_sse4_time / actual_rounds;
            double avg_avx2 = total_avx2_time / actual_rounds;
            
            // 计算加速比
            double speedup_sse2 = avg_serial / avg_sse2;
            double speedup_sse4 = avg_serial / avg_sse4;
            double speedup_avx2 = avg_serial / avg_avx2;
            
            // 打印结果行到控制台
            cout << dataSize << "\t" << pwLength << "\t\t" 
                 << fixed << setprecision(4) << avg_serial << "\t" 
                 << avg_sse2 << "\t" 
                 << avg_sse4 << "\t" 
                 << avg_avx2 << "\t" 
                 << setprecision(2) << speedup_sse2 << "x\t" 
                 << speedup_sse4 << "x\t" 
                 << speedup_avx2 << "x" << endl;
            
            // 写入结果到CSV文件
            csvFile << dataSize << "," << pwLength << "," 
                    << fixed << setprecision(4) << avg_serial << "," 
                    << avg_sse2 << "," 
                    << avg_sse4 << "," 
                    << avg_avx2 << "," 
                    << setprecision(2) << speedup_sse2 << "," 
                    << speedup_sse4 << "," 
                    << speedup_avx2 << endl;
        }
    }
    
    // 关闭CSV文件
    csvFile.close();
    
    cout << "\n=========================================================" << endl;
    cout << "测试完成! 结果已保存到 md5_performance_results.csv" << endl;
    
    return 0;
}