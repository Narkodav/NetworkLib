#pragma once
#include "Common.h"
#include "Socket.h"
#include "IOContext.h"

class ConnectionManager
{
private:
#ifdef _WIN32
	static inline WSAData wsaData;
	static inline size_t managerCounter = 0;
#endif

	Socket m_acceptSocket;
	int m_port;

	MT::ThreadPool m_pool;
	std::atomic<bool> m_shouldRun;
	size_t m_threadCount;

public:
	ConnectionManager(int port) :
		m_port(port), m_acceptSocket()
	{
#ifdef _WIN32
		++managerCounter;
		if (managerCounter == 1)
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
				throw std::runtime_error("WSAStartup failed");
			}
#endif
		m_pool.init(m_threadCount);
		m_acceptSocket.bind("0.0.0.0", port);
		m_acceptSocket.listen();
	}
	~ConnectionManager() {
		m_acceptSocket.close();
	};

	void asyncAccept(std::function<void(Socket&&)> acceptCallback)
	{
		m_pool.pushTask([this, callback = std::move(acceptCallback)]() {
			try {
				callback(std::move(m_acceptSocket.accept()));
			}
			catch (const std::exception& e) {
				// Handle accept errors
			}
			});

	}
};

