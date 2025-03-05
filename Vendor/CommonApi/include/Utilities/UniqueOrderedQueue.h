#pragma once
#include "../Namespaces.h"

#include <unordered_set>
#include <queue>
#include <vector>
#include <stdexcept>

namespace Utilities
{
	template<typename T, typename Compare = std::less<T>, typename Hash = std::hash<T>>

	class UniqueOrderedQueue
	{
	private:
		std::priority_queue<T, std::vector<T>, Compare> m_queue;
		std::unordered_set<T, Hash> m_elements;

	public:

		void push(const T& value)
		{
			if (m_elements.find(value) == m_elements.end())
			{
				m_queue.push(value);
				m_elements.insert(value);
			}
		}

		void pop()
		{
			if (empty())  // use empty() check
				throw std::runtime_error("Queue is empty");  // throw instead of silent return

			m_elements.erase(m_queue.top());
			m_queue.pop();
		}

		const T& top()
		{
			if (empty())
				throw std::runtime_error("Queue is empty");
			return m_queue.top();
		}

		bool empty() const { return m_queue.empty(); };
		size_t size() const { return m_queue.size(); };
		void clear() {
			while (!m_queue.empty())
			{
				m_queue.pop();
			}
			m_elements.clear();
		};
	};

}