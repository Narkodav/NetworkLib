#pragma once
#include "../Namespaces.h"

#include <mutex>
#include <condition_variable>
#include <queue>

namespace MultiThreading
{
    template <typename T>
    class Queue
    {
    private:
        std::queue<T> queue;
        mutable std::mutex mutex;
        std::condition_variable_any notEmpty;

    public:
        Queue() = default;

        Queue(Queue&& other) noexcept {
            std::lock_guard<std::mutex> lock(other.mutex);
            deque = std::exchange(other.deque, std::queue<T>());
        }

        Queue& operator=(Queue&& other) noexcept {
            if (this != &other) {
                std::scoped_lock locks(mutex, other.mutex);
                deque = std::exchange(other.deque, std::queue<T>());
            }
            return *this;
        }

        Queue(const Queue& other)
        {
            std::lock_guard<std::mutex> lockOther(other.mutex);
            queue = other.queue;
        }

        Queue& operator=(const Queue& other)
        {
            if (this != &other) {
                std::scoped_lock locks(mutex, other.mutex);
                queue = other.queue;
            }
            return *this;
        }

        // Push an item to the queue
        void push(T value) {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::move(value));
            notEmpty.notify_one();
        }

        // Pop an item from the queue
        bool pop(T& value) {
            std::lock_guard<std::mutex> lock(mutex);
            if (queue.empty()) {
                return 0;
            }
            value = std::move(queue.front());
            queue.pop();
            return 1;
        }

        // Wait and pop an item from the queue
        void waitAndPop(T& value) {
            std::unique_lock<std::mutex> lock(mutex);

            if (!queue.empty()) {
                value = std::move(queue.front());
                queue.pop();
            }

            notEmpty.wait(lock, [this]() { return !queue.empty(); });
            value = std::move(queue.front());
            queue.pop();
        }

        template<typename Rep, typename Period>
        bool waitAndPopFor(T& value, const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock<std::mutex> lock(mutex);

            if (!queue.empty()) {
                value = std::move(queue.front());
                queue.pop();
                return true;
            }

            if (notEmpty.wait_for(lock, timeout, [this]() { return !queue.empty(); }))
            {
                value = std::move(queue.front());
                queue.pop();
                return true;
            }
            return false;
        }

        // Check if queue is empty
        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex);
            return queue.empty();
        }

        // Get queue size
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex);
            return queue.size();
        }

        T front() const {
            std::lock_guard<std::mutex> lock(mutex);
            return queue.front();
        }

        T back() const {
            std::lock_guard<std::mutex> lock(mutex);
            return queue.back();
        }
    };
}
