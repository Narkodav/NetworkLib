#pragma once
#include "../Namespaces.h"

#include <shared_mutex>

namespace MultiThreading
{
	template <typename T>
	class Synchronized
	{

	public:
		struct WriteAccess
		{
			// Default constructor
			WriteAccess() : data(nullptr), lock() {};
			~WriteAccess() = default;

			// Main constructors
			WriteAccess(T& data, std::shared_mutex& mutex)
				: data(&data), lock(mutex) {}

			WriteAccess(T& data, std::unique_lock<std::shared_mutex>& lock)
				: data(&data), lock(std::move(lock)) {
				if (!this->lock.owns_lock()) {
					throw std::runtime_error("Lock must be owned");
				}
			}

			// Move constructor
			WriteAccess(WriteAccess&& other) noexcept
				: data(other.data), lock(std::move(other.lock)) {}

			// Move assignment
			WriteAccess& operator=(WriteAccess&& other) noexcept {
				if (this != &other) {
					data = other.data;
					lock = std::move(other.lock);
				}
				return *this;
			}

			// Delete copy operations
			WriteAccess(const WriteAccess&) = delete;
			WriteAccess& operator=(const WriteAccess&) = delete;

			inline T& operator*() {
				if (!data) throw std::runtime_error("Dereferencing a null pointer");
				return *data;
			};

			inline const T& operator*() const {
				if (!data) throw std::runtime_error("Dereferencing a null pointer");
				return *data;
			};

			inline T* operator->() {
				if (!data) throw std::runtime_error("Dereferencing a null pointer");
				return data;
			};

			inline const T* operator->() const {
				if (!data) throw std::runtime_error("Dereferencing a null pointer");
				return data;
			};
		private:
			T* data;
			std::unique_lock<std::shared_mutex> lock;
		};

		struct ReadAccess
		{
			// Default constructor
			ReadAccess() : data(nullptr), lock() {};
			~ReadAccess() = default;

			// Main constructors
			ReadAccess(const T& data, std::shared_mutex& mutex)
				: data(&data), lock(mutex) {}

			ReadAccess(const T& data, std::shared_lock<std::shared_mutex>& lock)
				: data(&data), lock(std::move(lock)) {
				if (!this->lock.owns_lock()) {
					throw std::runtime_error("Lock must be owned");
				}
			}

			// Move constructor
			ReadAccess(ReadAccess&& other) noexcept
				: data(other.data), lock(std::move(other.lock)) {}

			// Move assignment
			ReadAccess& operator=(ReadAccess&& other) noexcept {
				if (this != &other) {
					data = other.data;
					lock = std::move(other.lock);
				}
				return *this;
			}

			// Delete copy operations
			ReadAccess(const ReadAccess&) = delete;
			ReadAccess& operator=(const ReadAccess&) = delete;

			inline const T& operator*() const {
				if (!data) throw std::runtime_error("Dereferencing a null pointer");
				return *data;
			};

			inline const T* operator->() const {
				if (!data) throw std::runtime_error("Dereferencing a null pointer");
				return data;
			};

		private:
			const T* data;
			std::shared_lock<std::shared_mutex> lock;
		};

	private:

		T m_data;
		mutable std::shared_mutex m_mutex;

	public:

		// Constructor to allow conversion from non-const to const wrapper
		Synchronized() = default;

		inline WriteAccess getWriteAccess()
		{
			return WriteAccess(m_data, m_mutex);
		}

		inline ReadAccess getReadAccess() const
		{
			return ReadAccess(m_data, m_mutex);
		}

		inline bool tryWriteAccess(WriteAccess access)
		{
			std::unique_lock<std::shared_mutex> lock(m_mutex, std::try_to_lock);
			if (!lock.owns_lock()) {
				return 0;
			}
			access = WriteAccess(m_data, lock);
			return 1;
		}

		inline bool tryReadAccess(ReadAccess access) const
		{
			std::shared_lock<std::shared_mutex> lock(m_mutex, std::try_to_lock);
			if (!lock.owns_lock()) {
				return 0;
			}
			access = ReadAccess(m_data, lock);
			return 1;
		}
	};
}

