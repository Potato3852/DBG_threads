#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

// Глобальная переменная для гонки
int shared_counter = 0;

// Функция, которая вызывает data race
void increment_without_lock(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        shared_counter++;  // Классический data race!
    }
}

// Функция с мьютексом (правильная)
void increment_with_lock(int iterations, std::mutex& mtx) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        shared_counter++;
    }
}

int main(int argc, char* argv[]) {
    const int ITERATIONS = 1000000;
    int NUM_THREADS = 4;
    if (argc > 1) NUM_THREADS = std::atoi(argv[1]);
    
    std::cout << "=== Race Condition Demo ===" << std::endl;
    std::cout << "Threads: " << NUM_THREADS << std::endl;
    std::cout << "Iterations per thread: " << ITERATIONS << std::endl;
    
    // ТЕСТ 1: Без синхронизации (будет гонка)
    std::cout << "\n[TEST 1] Without synchronization (expecting race):" << std::endl;
    shared_counter = 0;
    
    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(increment_without_lock, ITERATIONS);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Expected value: " << (NUM_THREADS * ITERATIONS) << std::endl;
    std::cout << "Actual value:   " << shared_counter << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    
    if (shared_counter != NUM_THREADS * ITERATIONS) {
        std::cout << "🚨 DATA RACE DETECTED! Loss: " 
                  << (NUM_THREADS * ITERATIONS - shared_counter) << " increments" << std::endl;
    }
    
    // ТЕСТ 2: С синхронизацией (правильно)
    std::cout << "\n[TEST 2] With mutex (correct):" << std::endl;
    shared_counter = 0;
    threads.clear();
    std::mutex mtx;
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(increment_with_lock, ITERATIONS, std::ref(mtx));
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Expected value: " << (NUM_THREADS * ITERATIONS) << std::endl;
    std::cout << "Actual value:   " << shared_counter << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    
    if (shared_counter == NUM_THREADS * ITERATIONS) {
        std::cout << "✅ No data race" << std::endl;
    }
    
    std::cout << "\nPerformance impact: " 
              << (duration.count() > 0 ? "Mutex is " + std::to_string(duration.count() / 10.0) + "x slower" : "Similar")
              << std::endl;
    
    return 0;
}