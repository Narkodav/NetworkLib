#pragma once
#include "../Namespaces.h"
#include "ThreadPool.h"

#include <atomic>
#include <functional>

namespace MultiThreading
{
	class TaskCoordinator
	{
	private:
		unsigned int m_maxPendingTasks;
		std::atomic<unsigned int> m_pendingTasks;
		ThreadPool& m_threadPoolHandle;

	public:
		TaskCoordinator(ThreadPool& threadPool, unsigned int maxPendingTasks) :
			m_threadPoolHandle(threadPool), m_maxPendingTasks(maxPendingTasks), m_pendingTasks(0) {};

		bool tryAddTask(std::function<void()> task)
		{
			if (m_pendingTasks >= m_maxPendingTasks)
				return 0;
			m_pendingTasks++;
			m_threadPoolHandle.pushTask([this, f = std::move(task)] {
				f();
				m_pendingTasks--;
				});
			return 1;
		}

		bool canAddTask()
		{
			if (m_pendingTasks >= m_maxPendingTasks)
				return 0;
			return 1;
		}

		inline ThreadPool& getPoolHandle() const {
			return m_threadPoolHandle;
		};

	};
}
