#pragma once
#include "../Namespaces.h"
#include "ThreadPool.h"
#include "Synchronized.h"

#include <set>

namespace MultiThreading
{
	template <typename T, typename Hash = std::less<T>>
	class UniqueTaskCoordinator
	{
	private:
		unsigned int m_maxPendingTasks;
		ThreadPool& m_threadPoolHandle;
		Synchronized<std::set<T, Hash>> m_pendingTasks;
	public:
		UniqueTaskCoordinator(ThreadPool& threadPool, unsigned int maxPendingTasks) :
			m_threadPoolHandle(threadPool),
			m_maxPendingTasks(maxPendingTasks),
			m_pendingTasks(Synchronized<std::vector<T>>()) {
			m_pendingTasks.getWriteAccess()->reserve(maxPendingTasks);
		}

		bool tryAddTask(std::function<void()> task, T identifier)
		{
			{
				auto access = m_pendingTasks.getWriteAccess();
				if (access->size() >= m_maxPendingTasks ||
					access->find(identifier) != access->end())
					return 0;
				access->insert(identifier);
			}

			m_threadPoolHandle.pushTask([this, f = std::move(task), identifier] {
				f();
				auto access = m_pendingTasks.getWriteAccess();
				access->erase(identifier);
				});
			return 1;
		}

		bool canAddTask(T identifier)
		{
			auto access = m_pendingTasks.getWriteAccess();
			if (access->size() >= m_maxPendingTasks ||
				access->find(identifier) != access->end())
				return 0;
			return 1;
		}

		bool canAddTask()
		{
			auto access = m_pendingTasks.getWriteAccess();
			if (access->size())
				return 0;
			return 1;
		}

		inline ThreadPool& getPoolHandle() const {
			return m_threadPoolHandle;
		};
	};
}
