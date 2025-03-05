#pragma once
#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include "../Namespaces.h"
#include "Synchronized.h"
#include "PointerControlBlock.h"

#include <type_traits>
#include <vector>
#include <set>
#include <atomic>
#include <shared_mutex>
#include <stdexcept>

//Memory pool
namespace MultiThreading
{
	template <typename T>
	class MemoryPool
	{
	public:
		using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
		struct Iterator
		{
			T* ptr;
			unsigned int index;
			MemoryPool* pool = nullptr;
			Iterator(T* p = nullptr, unsigned int i = 0, MemoryPool* pool = nullptr) :
				ptr(p), index(i), pool(pool) {};
		};

	private:
		std::vector<Storage> m_data;
		std::vector<size_t> m_freeChunks;
		std::set<size_t> m_allocatedChunks;
		Storage* dataPtr = nullptr;

		std::atomic<bool> m_isSet = 0;
		mutable std::shared_mutex m_poolMutex;

		static size_t getIndexFromPointer(T* ptr, std::vector<Storage> pool) {
			if (!ptr) return std::numeric_limits<std::size_t>::max();

			std::ptrdiff_t diff = reinterpret_cast<const char*>(ptr); -
				reinterpret_cast<const char*>(dataPtr);

			//if (diff < 0 || static_cast<size_t>(diff) >= m_data.size()) {
			//	throw std::out_of_range("Pointer not from this pool");
			//}

			return static_cast<size_t>(diff) / sizeof(Storage);
		}

		Iterator allocate()
		{
			std::lock_guard<std::shared_mutex> lock(m_poolMutex);

			if (!m_isSet) {
				throw std::runtime_error("Memory pool not initialized");
			}

			if (m_freeChunks.empty()) {
				throw std::bad_alloc();
			}

			unsigned int index = m_freeChunks.back();

			if (index >= m_data.size()) {
				throw std::out_of_range("Invalid pool index");
			}

			m_freeChunks.pop_back();
			m_allocatedChunks.insert(index);
			if (m_freeChunks.size() + m_allocatedChunks.size() != m_data.size())
				__debugbreak();
			return Iterator(new (&m_data[index]) T(), index, this);
		}

		void deallocate(Iterator& iterator)
		{
			std::lock_guard<std::shared_mutex> lock(m_poolMutex);
			if (iterator.ptr == nullptr)
			{
				__debugbreak();
				return;
			}

			m_freeChunks.push_back(iterator.index);
			m_allocatedChunks.erase(iterator.index);
			iterator.ptr->~T();
			if (m_freeChunks.size() + m_allocatedChunks.size() != m_data.size())
				__debugbreak();
		}

	public:

		MemoryPool() : m_isSet(0) {}
		MemoryPool(unsigned int size) : m_isSet(0) { set(size); }
		~MemoryPool() { clear(); }

		MemoryPool(const MemoryPool&) = delete;
		MemoryPool& operator=(const MemoryPool&) = delete;

		MemoryPool(MemoryPool&&) noexcept = default;
		MemoryPool& operator=(MemoryPool&&) noexcept = default;

		class SharedPointer;
		class UniquePointer;
		class WeakPointer;

		void set(unsigned int size)
		{
			if (size == 0) {
				throw std::invalid_argument("Pool size cannot be zero");
			}

			if (m_isSet.load())
				clear();
			std::lock_guard<std::shared_mutex> lock(m_poolMutex);

			try {
				m_data.resize(size);
				m_freeChunks.resize(size);
				for (unsigned int i = 0; i < size; i++) {
					m_freeChunks[i] = i;
				}
				dataPtr = m_data.data();
				m_isSet = 1;
			}
			catch (...) {
				m_isSet = 0;
				m_data.clear();
				m_freeChunks.clear();
				throw;
			}
		}

		void clear()
		{
			std::lock_guard<std::shared_mutex> lock(m_poolMutex);
			m_isSet = 0;
			for (size_t i = 0; i < m_data.size(); ++i) {
				if (std::find(m_freeChunks.begin(), m_freeChunks.end(), i)
					== m_freeChunks.end()) {
					reinterpret_cast<T*>(&m_data[i])->~T();
				}
			}
			m_data.clear();
			m_freeChunks.clear();
			dataPtr = nullptr;
		}

		unsigned int getAllocatedSize() const
		{
			std::shared_lock<std::shared_mutex> lock(m_poolMutex);
			return m_allocatedChunks.size();
		}

		unsigned int getFreeSize() const
		{
			std::shared_lock<std::shared_mutex> lock(m_poolMutex);
			return m_freeChunks.size();
		}

		bool isInitialized() const {
			std::shared_lock<std::shared_mutex> lock(m_poolMutex);
			return m_isSet.load();
		}

		size_t capacity() const {
			std::shared_lock<std::shared_mutex> lock(m_poolMutex);
			return m_data.size();
		}

		friend class SharedPointer;
		friend class UniquePointer;

		SharedPointer makeShared()
		{
			SharedPointer ptr = allocate();
			return ptr;
		}

		SharedPointer makeShared(const T& data)
		{
			SharedPointer ptr = allocate();
			*ptr = data;
			return ptr;
		}

		UniquePointer makeUnique()
		{
			UniquePointer ptr = allocate();
			return ptr;
		}

		UniquePointer makeUnique(const T& data)
		{
			UniquePointer ptr = allocate();
			*ptr = data;
			return ptr;
		}

	};
}

#include "SharedPointer.h"
#include "WeakPointer.h"
#include "UniquePointer.h"

#endif //MEMORYPOOL_H