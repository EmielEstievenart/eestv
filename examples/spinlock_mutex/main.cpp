#include <iostream>
#include <thread>
#include "eestv/spinlock_mutex.hpp"

using eestv::SpinlockMutex;

int main() {
    std::cout << "Hello, World!" << std::endl;

    SpinlockMutex mutex;

    std::thread t1([&mutex]() {
        mutex.lock();
        std::cout << "Thread 1: Mutex is locked." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        mutex.unlock();
        std::cout << "Thread 1: Mutex is unlocked." << std::endl;
    });

    std::thread t2([&mutex]() {
        mutex.lock();
        std::cout << "Thread 2: Mutex is locked." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        mutex.unlock();
        std::cout << "Thread 2: Mutex is unlocked." << std::endl;
    });

    t1.join();
    t2.join();

    return 0;
}