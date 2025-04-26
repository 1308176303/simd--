#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5.h"
#include <iomanip>
using namespace std;
using namespace chrono;

// 编译指令如下
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main -O1
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main -O2

int main()
{
    double time_hash = 0;         // 总哈希时间
    double time_hash_serial = 0;  // 串行哈希累计时间
    double time_hash_simd = 0;    // 并行哈希累计时间
    double time_guess = 0;        // 哈希和猜测的总时长
    double time_train = 0;        // 模型训练的总时长
    PriorityQueue q;
    auto start_train = system_clock::now();
    q.m.train("/guessdata/Rockyou-singleLined-full.txt");
    q.m.order();
    auto end_train = system_clock::now();
    auto duration_train = duration_cast<microseconds>(end_train - start_train);
    time_train = double(duration_train.count()) * microseconds::period::num / microseconds::period::den;

    q.init();
    cout << "here" << endl;
    int curr_num = 0;
    auto start = system_clock::now();
    // 由于需要定期清空内存，我们在这里记录已生成的猜测总数
    int history = 0;
    // std::ofstream a("./files/results.txt");
    while (!q.priority.empty())
    {
        q.PopNext();
        q.total_guesses = q.guesses.size();
        if (q.total_guesses - curr_num >= 100000)
        {
            cout << "Guesses generated: " <<history + q.total_guesses << endl;
            curr_num = q.total_guesses;

            // 在此处更改实验生成的猜测上限
            int generate_n=10000000;
            if (history + q.total_guesses > 10000000)
            {
                auto end = system_clock::now();
                auto duration = duration_cast<microseconds>(end - start);
                time_guess = double(duration.count()) * microseconds::period::num / microseconds::period::den;
                cout << "Guess time:" << time_guess - time_hash << "seconds"<< endl;
                cout << "Hash time (serial):" << time_hash_serial << "seconds"<<endl;
                cout << "Hash time (SIMD):" << time_hash_simd << "seconds"<<endl;
                cout << "Hash speedup:" << time_hash_serial / time_hash_simd << "x" << endl;
                cout << "Train time:" << time_train <<"seconds"<<endl;
                break;
            }
        }
        // 为了避免内存超限，我们在q.guesses中口令达到一定数目时，将其中的所有口令取出并且进行哈希
        // 然后，q.guesses将会被清空。为了有效记录已经生成的口令总数，维护一个history变量来进行记录
        if (curr_num > 1000000)
        {
            auto start_hash = system_clock::now();
            
            // ==================== 串行版本 ====================
            bit32 state[4];
            auto start_serial = system_clock::now();
            for (string pw : q.guesses)
            {
                MD5Hash(pw, state);
            }
            
            // 计算串行版本时间
            auto end_serial = system_clock::now();
            auto duration_serial = duration_cast<microseconds>(end_serial - start_serial);
            double time_serial = double(duration_serial.count()) * microseconds::period::num / microseconds::period::den;
            time_hash_serial += time_serial; // 累加串行时间
            
            // ==================== SIMD并行版本 ====================
            auto start_simd = system_clock::now();
            
            // 1. 预分配大量密码的内存 - 使用连续数组提高访问效率
            const size_t pw_count = q.guesses.size();
            string* passwords = new string[pw_count];
            
            // 2. 以连续方式存储所有密码 - 提高缓存命中率
            size_t idx = 0;
            for (const string& pw : q.guesses) {
                passwords[idx++] = pw;
            }
            
            // 3. 使用对齐内存分配哈希结果数组 - 提高SIMD性能
            bit32** hash_results = new bit32*[pw_count];
            for (size_t i = 0; i < pw_count; i++) {
                // 16字节对齐的内存分配（SIMD要求）
                void* ptr = nullptr;
                if (posix_memalign(&ptr, 16, 4 * sizeof(bit32)) != 0) {
                    hash_results[i] = new bit32[4];
                } else {
                    hash_results[i] = static_cast<bit32*>(ptr);
                }
            }
            
            // 4. 调用SIMD版本进行批量计算 - 充分利用SIMD指令
            MD5Hash_SIMD(passwords, pw_count, hash_results);
            
            // 5. 释放分配的内存
            delete[] passwords;
            for (size_t i = 0; i < pw_count; i++) {
                if (hash_results[i]) {
                    free(hash_results[i]);  // 使用free释放对齐内存
                }
            }
            delete[] hash_results;
            
            // 计算SIMD版本时间并累加
            auto end_simd = system_clock::now();
            auto duration_simd = duration_cast<microseconds>(end_simd - start_simd);
            double time_simd = double(duration_simd.count()) * microseconds::period::num / microseconds::period::den;
            time_hash_simd += time_simd; // 累加并行时间
            
            // 输出本轮性能比较结果
            cout << "密码数量: " << pw_count << endl;
            cout << "串行哈希时间: " << time_serial << " 秒" << endl;
            cout << "SIMD哈希时间: " << time_simd << " 秒" << endl;
            cout << "加速比: " << time_serial / time_simd << "x" << endl;
            
            // 计算总哈希时间（用于计算猜测时间）
            auto end_hash = system_clock::now();
            auto duration = duration_cast<microseconds>(end_hash - start_hash);
            time_hash += double(duration.count()) * microseconds::period::num / microseconds::period::den;

            // 记录已经生成的口令总数
            history += curr_num;
            curr_num = 0;
            q.guesses.clear();
        }
    }
}