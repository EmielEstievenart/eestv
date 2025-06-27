#pragma once

#include <atomic>

namespace eestv {

    class SpinlockMutex {
        public: 

        SpinlockMutex(){}

        void lock() {
            while (_flag.test_and_set(std::memory_order_acquire)) {
                // spin-wait
            }
        }

        void unlock() {
            _flag.clear(std::memory_order_release);
        }

        private:
        std::atomic_flag _flag{ATOMIC_FLAG_INIT};
    };
}