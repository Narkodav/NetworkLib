#pragma once
#include "Common.h"
#include "Socket.h"

///**
// * @brief IOContext manages asynchronous I/O operations and thread pooling
// *
// * Important Note: Unlike traditional async I/O implementations (like Boost.Asio),
// * this IOContext begins processing requests immediately upon construction. The thread
// * pool is initialized and begins accepting tasks as soon as the IOContext is created.
// *
// * The maintenanceLoop() method is NOT required to begin processing I/O operations. It only performs
// * maintenance tasks such as:
// * - Thread pool monitoring
// * - Pool size adjustments
// * - Resource cleanup
// * - Performance statistics gathering
// *
// * Example usage:
// * @code
// * IOContext io;  // Thread pool starts here, ready to process requests
// *
// * // Acceptor immediately starts accepting connections
// * Acceptor acceptor(io, 8080);
// * acceptor.asyncAccept([](Socket&& client) {
// *     // This executes in thread pool, regardless of whether maintenanceLoop() is called
// *     handleClient(std::move(client));
// * });
// *
// * // Optional: maintenanceLoop
// * io.maintenanceLoop();  // Only needed if you want the maintenance features
// * @endcode
// */

class IOContext
{
private:

#ifdef _WIN32
	static inline WSAData wsaData;
	static inline size_t contextCounter = 0;
#endif
	MT::ThreadPool m_pool;
	std::atomic<bool> m_shouldRun;
	MT::Deque<std::pair<Socket, std::function<void(Socket&&)>>> m_acceptCallbackQueue;
	size_t m_threadCount;

public:

	IOContext(size_t threadCount = std::thread::hardware_concurrency() * 4) :
		m_threadCount(threadCount) {
#ifdef _WIN32
		++contextCounter;
		if (contextCounter == 1)
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
				throw std::runtime_error("WSAStartup failed");
			}
#endif
		m_pool.init(m_threadCount);
	};

	void run() {
		m_shouldRun = true;
		std::pair<Socket, std::function<void(Socket&&)>> buffer;
		//std::function<void(Socket&&)> task;

		while (m_shouldRun.load())
		{
			m_acceptCallbackQueue.waitAndPopFront(buffer);
			//Socket socket = std::move(buffer.first);
			//task = buffer.second;
			//task(std::move(socket));
			buffer.second(std::move(buffer.first));
			std::this_thread::yield();
		}
	}

	void stop() {
		m_shouldRun = false;
		m_pool.shutdown();
	}

	void post(std::function<void()> task) {
		m_pool.pushTask(task);
	}

	void postAcceptCallback (Socket&& socket, std::function<void(Socket&&)> task) {
		m_acceptCallbackQueue.pushBack(std::make_pair(std::move(socket), task));
	}

	~IOContext() {
#ifdef _WIN32
		--contextCounter;
		if (contextCounter == 0)
			WSACleanup();
#endif
	}

};

