#pragma once
#include "../Namespaces.h"

#include <vector>
#include <mutex>

namespace MultiThreading
{
    template <typename T>
    class Vector
    {
    private:
        std::vector<T> m_vector;
        mutable std::mutex m_mutex;

    public:
        Vector() = default;

        Vector(Vector&& other) noexcept {
            std::lock_guard<std::mutex> lock(other.m_mutex);
            m_vector = std::move(other.m_vector);
            other.m_vector.clear();
        }

        Vector& operator=(Vector&& other) noexcept {
            if (this != &other) {
                std::scoped_lock locks(m_mutex, other.m_mutex);
                m_vector = std::move(other.m_vector);
                other.m_vector.clear();
            }
            return *this;
        }

        Vector(const Vector& other)
        {
            std::lock_guard<std::mutex> lockOther(other.m_mutex);
            m_vector = other.m_vector;
        }

        Vector& operator=(const Vector& other)
        {
            if (this != &other) {
                std::scoped_lock locks(m_mutex, other.m_mutex);
                m_vector = other.m_vector;
            }
            return *this;
        }

        // Add element
        void pushBack(const T& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.push_back(value);
        }

        void pushBack(T&& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.push_back(std::move(value));
        }

        // For non-bool types
        template<typename U = T>
        typename std::enable_if<!std::is_same<U, bool>::value, U&>::type
            operator[](size_t index) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // For bool type
        template<typename U = T>
        typename std::enable_if<std::is_same<U, bool>::value, typename std::vector<bool>::reference>::type
            operator[](size_t index) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // Const access for non-bool types
        template<typename U = T>
        typename std::enable_if<!std::is_same<U, bool>::value, const U&>::type
            operator[](size_t index) const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // Const access for bool type
        template<typename U = T>
        typename std::enable_if<std::is_same<U, bool>::value, bool>::type
            operator[](size_t index) const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // Resize
        void resize(size_t newSize) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.resize(newSize);
        }

        void resize(size_t newSize, const T& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.resize(newSize, value);
        }

        // Get size
        size_t size() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector.size();
        }

        // Clear
        void clear() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.clear();
        }

        // Optional: Get underlying vector (careful with this!)
        std::vector<T> getCopy() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector;
        }
    };
}
