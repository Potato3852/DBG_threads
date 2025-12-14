#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mutex1, mutex2;

// Функция, которая может вызвать deadlock (неправильный порядок)
void thread1_deadlock() {
    std::cout << "Thread 1: Locking mutex1..." << std::endl;
    mutex1.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Увеличиваем шанс deadlock
    
    std::cout << "Thread 1: Trying to lock mutex2..." << std::endl;
    mutex2.lock();
    
    std::cout << "Thread 1: Critical section (should not reach here if deadlock)" << std::endl;
    
    mutex2.unlock();
    mutex1.unlock();
}

void thread2_deadlock() {
    std::cout << "Thread 2: Locking mutex2..." << std::endl;
    mutex2.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    std::cout << "Thread 2: Trying to lock mutex1..." << std::endl;
    mutex1.lock();
    
    std::cout << "Thread 2: Critical section (should not reach here if deadlock)" << std::endl;
    
    mutex1.unlock();
    mutex2.unlock();
}

// Функция без deadlock (правильный порядок)
void thread1_safe() {
    std::lock(mutex1, mutex2); // Lock both at once
    std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
    
    std::cout << "Thread 1: Critical section (safe)" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void thread2_safe() {
    std::lock(mutex1, mutex2); // Тот же порядок!
    std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
    
    std::cout << "Thread 2: Critical section (safe)" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {
    std::cout << "=== Deadlock Demo ===" << std::endl;
    
    // ТЕСТ 1: С deadlock
    std::cout << "\n[TEST 1] Causing deadlock (circular wait):" << std::endl;
    
    auto t1 = std::thread(thread1_deadlock);
    auto t2 = std::thread(thread2_deadlock);
    
    // Даем потокам время на deadlock
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Проверяем, живы ли потоки
    bool t1_done = t1.joinable();
    bool t2_done = t2.joinable();
    
    if (t1_done && t2_done) {
        // Потоки завершились - нет deadlock
        t1.join();
        t2.join();
        std::cout << "✅ No deadlock occurred (lucky timing)" << std::endl;
    } else {
        std::cout << "\n🚨 DEADLOCK DETECTED! Threads are stuck." << std::endl;
        std::cout << "Thread 1 joinable: " << (t1.joinable() ? "Yes" : "No") << std::endl;
        std::cout << "Thread 2 joinable: " << (t2.joinable() ? "Yes" : "No") << std::endl;
        
        // Отделяем потоки (оставляем в deadlock для анализа)
        t1.detach();
        t2.detach();
        std::cout << "Threads detached. Process will exit with deadlock." << std::endl;
    }
    
    // ТЕСТ 2: Без deadlock
    std::cout << "\n[TEST 2] Deadlock-free version:" << std::endl;
    
    auto t3 = std::thread(thread1_safe);
    auto t4 = std::thread(thread2_safe);
    
    t3.join();
    t4.join();
    
    std::cout << "✅ Both threads completed successfully" << std::endl;
    
    return 0;
}