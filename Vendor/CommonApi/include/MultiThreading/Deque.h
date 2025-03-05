#pragma once
#include "../Namespaces.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <chrono>

namespace MultiThreading
{
    template <typename T>
    class Deque
    {
    private:
        std::deque<T> deque;
        mutable std::mutex mutex;
        std::condition_variable_any notEmpty;

    public:
        Deque() = default;

        Deque(Deque&& other) noexcept {
            std::lock_guard<std::mutex> lock(other.mutex);
            deque = std::exchange(other.deque, std::deque<T>());
        }

        Deque& operator=(Deque&& other) noexcept {
            if (this != &other) {
                std::scoped_lock locks(mutex, other.mutex);
                deque = std::exchange(other.deque, std::deque<T>());
            }
            return *this;
        }

        Deque(const Deque& other)
        {
            std::lock_guard<std::mutex> lockOther(other.mutex);
            deque = other.deque;
        }

        Deque& operator=(const Deque& other)
        {
            if (this != &other) {
                std::scoped_lock locks(mutex, other.mutex);
                deque = other.deque;
            }
            return *this;
        }


        void pushBack(T value) {
            std::lock_guard<std::mutex> lock(mutex);
            deque.push_back(std::move(value));
            notEmpty.notify_one();
        }

        void pushFront(T value) {
            std::lock_guard<std::mutex> lock(mutex);
            deque.push_front(std::move(value));
            notEmpty.notify_one();
        }

        bool popBack(T& value) {
            std::lock_guard<std::mutex> lock(mutex);
            if (deque.empty()) {
                return 0;
            }
            value = std::move(deque.back());
            deque.pop_back();
            return 1;
        }

        bool popFront(T& value) {
            std::lock_guard<std::mutex> lock(mutex);
            if (deque.empty()) {
                return 0;
            }
            value = std::move(deque.front());
            deque.pop_front();
            return 1;
        }

        // Wait and pop an item from the queue
        void waitAndPopFront(T& value) {
            std::unique_lock<std::mutex> lock(mutex);

            if (!deque.empty()) {
                value = std::move(deque.front());
                deque.pop_front();
            }

            notEmpty.wait(lock, [this]() { return !deque.empty(); });
            value = std::move(deque.front());
            deque.pop_front();
        }

        void waitAndPopBack(T& value) {
            std::unique_lock<std::mutex> lock(mutex);

            if (!deque.empty()) {
                value = std::move(deque.back());
                deque.pop_back();
            }

            notEmpty.wait(lock, [this]() { return !deque.empty(); });
            value = std::move(deque.back());
            deque.pop_back();
        }

        template<typename Rep, typename Period>
        bool waitAndPopFrontFor(T& value, const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock<std::mutex> lock(mutex);

            if (!deque.empty()) {
                value = std::move(deque.front());
                deque.pop_front();
                return true;
            }

            if (notEmpty.wait_for(lock, timeout, [this]() { return !deque.empty(); }))
            {
                value = std::move(deque.front());
                deque.pop_front();
                return true;
            }
            return false;
        }

        template<typename Rep, typename Period>
        bool waitAndPopBackFor(T& value, const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock<std::mutex> lock(mutex);

            if (!deque.empty()) {
                value = std::move(deque.back());
                deque.pop_back();
                return true;
            }

            if (notEmpty.wait_for(lock, timeout, [this]() { return !deque.empty(); }))
            {
                value = std::move(deque.back());
                deque.pop_back();
                return true;
            }
            return false;
        }

        // Check if queue is empty
        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex);
            return deque.empty();
        }

        // Get queue size
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex);
            return deque.size();
        }

        T front() const {
            std::lock_guard<std::mutex> lock(mutex);
            return deque.front();
        }

        T back() const {
            std::lock_guard<std::mutex> lock(mutex);
            return deque.back();
        }
    };
}