#pragma once

#include <queue>
#include <list>
#include <atomic>

namespace Citron {
    template<typename Any>
    class list : public std::list<Any> {
    private:
        std::atomic_flag _lock = ATOMIC_FLAG_INIT;

    public:
        list() : std::list<Any>() {}
        iterator push_back(const Any & item) {
            while (_lock.test_and_set())
                continue;
            std::list<Any>::push_back(item);
            iterator it = end();
            --it;
            _lock.clear();
            return it;
        }
        void erase(iterator it) {
            while (_lock.test_and_set())
                continue;
            std::list<Any>::erase(it);
            _lock.clear();
        }
        inline bool test_and_set() {
            return _lock.test_and_set();
        }
        inline void clear_lock() {
            _lock.clear();
        }
    };

    template<typename Any>
    class queue : public std::queue<Any> {
    private:
        std::atomic_flag _lock = ATOMIC_FLAG_INIT;

    public:
        queue() : std::queue<Any>() {}
        void push(const Any & item) {
            while (_lock.test_and_set())
                continue;
            std::queue<Any>::push(item);
            _lock.clear();
        }
        void pop() {
            while (_lock.test_and_set())
                continue;
            std::queue<Any>::pop();
            _lock.clear();
        }
    };
}
