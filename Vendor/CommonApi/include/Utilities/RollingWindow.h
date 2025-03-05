#pragma once
#include "../Namespaces.h"

#include <list>

namespace Utilities
{
	template<typename T>

	class RollingWindow
	{
		std::list<T> m_window;
		std::size_t m_windowSize;

	public:
		RollingWindow() : m_windowSize(0) {};
		RollingWindow(size_t maxSize) : m_windowSize(maxSize) {};
		RollingWindow(size_t maxSize, const T value) : m_windowSize(maxSize) {
			m_window.resize(m_windowSize, value);
		};

		using iterator = typename std::list<T>::iterator;
		using const_iterator = typename std::list<T>::const_iterator;

		iterator begin() { return m_window.begin(); };
		iterator end() { return m_window.end(); };

		const_iterator begin() const { return m_window.begin(); };
		const_iterator end() const { return m_window.end(); };

		const_iterator cbegin() const { return m_window.cbegin(); };
		const_iterator cend() const { return m_window.cend(); };

		T& back() { return m_window.back(); };
		T& front() { return m_window.front(); };

		const T& back() const { return m_window.back(); };
		const T& front() const { return m_window.front(); };

		void add(const T& value) {
			if (m_windowSize == 0)
				return;
			if (m_window.size() >= m_windowSize) {
				m_window.pop_front();
			}
			m_window.push_back(value);
		};

		void resize(size_t size) {
			m_windowSize = size;
			while (m_window.size() > m_windowSize) {
				m_window.pop_front();
			}
		};

		void resize(size_t size, const T value) {
			if (size < 0)
				return;
			m_windowSize = size;
			m_window.resize(size, value);
		};

		size_t size() const { return m_window.size(); };
		bool empty() const { return m_window.empty(); };
		size_t maxSize() const { return m_windowSize; };
		void clear() { m_window.clear(); };
	};
}
