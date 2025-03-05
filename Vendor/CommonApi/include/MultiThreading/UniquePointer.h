#pragma once

#ifndef MEMORYPOOL_H
#error "Do not include this file directly, use MemoryPool.h instead"
#endif

//Shared pointer
namespace MultiThreading
{
	template <typename T>
	class MemoryPool<T>::UniquePointer
	{
	public:
		static const UniquePointer nullPtr;
	private:
		Iterator m_iterator;

		inline void cleanup()
		{
			if (*this != nullptr)
			{
				m_iterator.pool->deallocate(m_iterator);
				nullify();
			}

		}

		inline void nullify()
		{
			m_iterator = { nullptr, 0, nullptr };
		}

	public:
		UniquePointer() : m_iterator(Iterator{ nullptr, 0, nullptr }) {};

		UniquePointer(std::nullptr_t) : m_iterator(Iterator{ nullptr, 0, nullptr }) {};

		UniquePointer(const UniquePointer& other) = delete;

		UniquePointer(const Iterator& other)
		{
			m_iterator = other;
		};

		UniquePointer(UniquePointer&& other) noexcept :
			m_iterator(other.m_iterator)
		{
			other.nullify();
		}

		~UniquePointer()
		{
			cleanup();
		};

		UniquePointer& operator=(std::nullptr_t)
		{
			cleanup();
			return *this;
		};

		UniquePointer& operator=(const UniquePointer& other) = delete;

		UniquePointer& operator=(const Iterator& iter)
		{
			cleanup();
			m_iterator = iter;
			return *this;
		};

		UniquePointer& operator=(UniquePointer&& other) noexcept
		{
			if (this == &other)
				return *this;

			cleanup();

			m_iterator = other.m_iterator;

			other.nullify();
			return *this;
		}

		void reset() {
			cleanup();
		}

		inline T& operator*() {
			if (!m_iterator.ptr) throw std::runtime_error("Dereferencing a null pointer");
			return *m_iterator.ptr;
		};

		inline const T& operator*() const {
			if (!m_iterator.ptr) throw std::runtime_error("Dereferencing a null pointer");
			return *m_iterator.ptr;
		};

		inline T* operator->() {
			if (!m_iterator.ptr) throw std::runtime_error("Dereferencing a null pointer");
			return m_iterator.ptr;
		};

		inline const T* operator->() const {
			if (!m_iterator.ptr) throw std::runtime_error("Dereferencing a null pointer");
			return m_iterator.ptr;
		};

		explicit operator bool() const {
			return m_iterator.ptr != nullptr;
		}

		bool operator==(std::nullptr_t) const { return m_iterator.ptr == nullptr; };
		bool operator!=(std::nullptr_t) const { return m_iterator.ptr != nullptr; };
		bool operator==(const UniquePointer& other) const { return m_iterator.ptr == other.m_iterator.ptr; };
		bool operator!=(const UniquePointer& other) const { return m_iterator.ptr != other.m_iterator.ptr; };
	};

	template <typename T>
	const typename MemoryPool<T>::UniquePointer
		MemoryPool<T>::UniquePointer::nullPtr = UniquePointer();
}