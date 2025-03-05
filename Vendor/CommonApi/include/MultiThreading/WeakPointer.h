#pragma once

#ifndef MEMORYPOOL_H
#error "Do not include this file directly, use MemoryPool.h instead"
#endif

//Weak pointer
namespace MultiThreading
{
	template <typename T>
	class MemoryPool<T>::WeakPointer
	{
	public:
		static const WeakPointer nullPtr;
	private:
		Iterator m_iterator;
		Synchronized<PointerControlBlock>* m_controlBlock;

		//control block access is only here
		inline void cleanup()
		{
			bool isLast = false;
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				if (!--access->weakCounter && !access->sharedCounter)
					isLast = true;
			}
			if (isLast)
				delete m_controlBlock;
			nullify();
		}

		inline void nullify()
		{
			m_iterator = { nullptr, 0, nullptr };
			m_controlBlock = nullptr;
		}

	public:
		WeakPointer() :
			m_iterator(Iterator{ nullptr, 0, nullptr }),
			m_controlBlock(nullptr) {};

		WeakPointer(std::nullptr_t) :
			m_iterator(Iterator{ nullptr, 0, nullptr }),
			m_controlBlock(nullptr) {};

		WeakPointer(MemoryPool<T>::SharedPointer ptr) :
			m_iterator(ptr.m_iterator),
			m_controlBlock(ptr.m_controlBlock) {
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				access->weakCounter++;
			}
		};

		WeakPointer(const WeakPointer& other) :
			m_iterator(other.m_iterator),
			m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				access->weakCounter++;
			}
		};

		WeakPointer(WeakPointer&& other) noexcept :
			m_iterator(other.m_iterator),
			m_controlBlock(other.m_controlBlock)
		{
			other.nullify();
		}

		~WeakPointer()
		{
			cleanup();
		};

		WeakPointer& operator=(std::nullptr_t)
		{
			cleanup();
			return *this;
		};

		WeakPointer& operator=(const WeakPointer& other)
		{
			if (this == &other)
				return *this;

			cleanup();
			m_iterator = other.m_iterator;
			m_controlBlock = other.m_controlBlock;
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				access->weakCounter++;
			}
			return *this;
		};

		WeakPointer& operator=(WeakPointer&& other) noexcept
		{
			if (this == &other)
				return *this;

			cleanup();

			m_iterator = other.m_iterator;
			m_controlBlock = other.m_controlBlock;

			other.nullify();
			return *this;
		}

		void reset() {
			cleanup();
		}

		SharedPointer lock() const
		{
			if (m_controlBlock != nullptr) {
				auto access = m_controlBlock->getWriteAccess();

				// Check if shared count is still valid
				if (access->sharedCounter) {
					access->sharedCounter++;
					return SharedPointer(m_iterator, m_controlBlock);
				}
			}
			return SharedPointer();
		}

		bool expired() const noexcept
		{
			if (!m_controlBlock)
				return true;
			return !m_controlBlock->getReadAccess()->sharedCounter;
		}

		size_t getCount() const {
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				return access->weakCounter;
			}
			return 0;
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
		bool operator==(const WeakPointer& other) const { return m_iterator.ptr == other.m_iterator.ptr; };
		bool operator!=(const WeakPointer& other) const { return m_iterator.ptr != other.m_iterator.ptr; };
	};

	template <typename T>
	const typename MemoryPool<T>::WeakPointer MemoryPool<T>::WeakPointer::nullPtr = WeakPointer();
}