#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

// Сравнение разных подходов к синхронизации

void test_atomic(int iterations, int num_threads) {
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    
    auto work = [&counter, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(work);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Atomic counter: " << counter.load() 
              << " in " << duration.count() << " ms" << std::endl;
}

void test_mutex(int iterations) {
    int counter = 0;
    std::mutex mtx;
    std::vector<std::thread> threads;
    
    auto work = [&counter, &mtx, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            std::lock_guard<std::mutex> lock(mtx);
            counter++;
        }
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(work);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Mutex counter:  " << counter
              << " in " << duration.count() << " ms" << std::endl;
}

void test_lockfree(int iterations) {
    // Локальная переменная в каждом потоке + редукция в конце
    std::vector<std::thread> threads;
    std::vector<int> local_counters(4, 0);
    int final_counter = 0;
    
    auto work = [&local_counters, iterations](int thread_id) {
        int local_sum = 0;
        for (int i = 0; i < iterations; ++i) {
            local_sum++;
        }
        local_counters[thread_id] = local_sum;
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(work, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Редукция
    for (int val : local_counters) {
        final_counter += val;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Lock-free:      " << final_counter
              << " in " << duration.count() << " ms" << std::endl;
}

int main(int argc, char* argv[]) {
    const int ITERATIONS = 1000000;

    int num_threads = 4;
    if (argc > 1) num_threads = std::atoi(argv[1]);
    
    std::cout << "=== Optimal Concurrency Comparison ===" << std::endl;
    std::cout << "Iterations per thread: " << ITERATIONS << std::endl;
    std::cout << "Threads: 4" << std::endl;
    
    std::cout << "\nComparing synchronization methods:" << std::endl;
    
    test_atomic(ITERATIONS, num_threads);
    test_mutex(ITERATIONS);
    test_lockfree(ITERATIONS);
    
    std::cout << "\n🎯 Recommendations:" << std::endl;
    std::cout << "• Use atomics for simple counters" << std::endl;
    std::cout << "• Use mutexes for complex critical sections" << std::endl;
    std::cout << "• Use lock-free when possible (thread-local + reduction)" << std::endl;
    
    return 0;
}