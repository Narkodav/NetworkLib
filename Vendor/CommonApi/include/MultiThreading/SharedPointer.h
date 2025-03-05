#pragma once

#ifndef MEMORYPOOL_H
#error "Do not include this file directly, use MemoryPool.h instead"
#endif

//Shared pointer
namespace MultiThreading
{
	template <typename T>
	class MemoryPool<T>::SharedPointer
	{
	public:
		static const SharedPointer nullPtr;
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
				if (!--access->sharedCounter)
				{
					m_iterator.pool->deallocate(m_iterator);
					if (!access->weakCounter)
						isLast = true;
				}
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

		SharedPointer(const Iterator& iter, Synchronized<PointerControlBlock>* controlBlock)
		{
			m_iterator = iter;
			m_controlBlock = controlBlock;
		};

	public:
		SharedPointer() :
			m_iterator(Iterator{ nullptr, 0, nullptr }),
			m_controlBlock(nullptr) {};

		SharedPointer(std::nullptr_t) :
			m_iterator(Iterator{ nullptr, 0, nullptr }),
			m_controlBlock(nullptr) {};

		SharedPointer(const SharedPointer& other) :
			m_iterator(other.m_iterator),
			m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				access->sharedCounter++;
			}
		};

		SharedPointer(const Iterator& iter)
		{
			m_iterator = iter;
			m_controlBlock = new Synchronized<PointerControlBlock>;
		};

		SharedPointer(SharedPointer&& other) noexcept :
			m_iterator(other.m_iterator),
			m_controlBlock(other.m_controlBlock)
		{
			other.nullify();
		}

		~SharedPointer()
		{
			cleanup();
		};

		SharedPointer& operator=(std::nullptr_t)
		{
			cleanup();
			return *this;
		};

		SharedPointer& operator=(const SharedPointer& other)
		{
			if (this == &other)
				return *this;

			cleanup();
			m_iterator = other.m_iterator;
			m_controlBlock = other.m_controlBlock;
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				access->sharedCounter++;
			}
			return *this;
		};

		SharedPointer& operator=(const Iterator& other)
		{
			cleanup();
			m_iterator = other;
			m_controlBlock = new Synchronized<PointerControlBlock>;
			return *this;
		};

		SharedPointer& operator=(SharedPointer&& other) noexcept
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

		size_t getCount() const {
			if (m_controlBlock)
			{
				auto access = m_controlBlock->getWriteAccess();
				return access->sharedCounter;
			}
			return 0;
		}

		WeakPointer makeWeak() const {
			return WeakPointer(*this);
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
		bool operator==(const SharedPointer& other) const { return m_iterator.ptr == other.m_iterator.ptr; };
		bool operator!=(const SharedPointer& other) const { return m_iterator.ptr != other.m_iterator.ptr; };

		friend class WeakPointer;
	};

	template <typename T>
	const typename MemoryPool<T>::SharedPointer
		MemoryPool<T>::SharedPointer::nullPtr = SharedPointer();
}