#pragma once
#include "Common.h"
#include "Socket.h"


class IOContext
{
public:
	struct SessionData
	{
		size_t bytesSent = 0;
		size_t bytesReceived = 0;
		size_t iterationCount = 0;
	};

	using AcceptCallback = std::function<void(Network::Socket&&)>;
	using ParserCallback = std::function<void(size_t)>; //returns bytes sent
	using SessionCallback = std::function<void(SessionData)>; //returns total sent bytes and the number of iterations

private:
	MT::ThreadPool m_pool;
	std::atomic<bool> m_shouldRun;
	MT::Deque<std::pair<Network::Socket, AcceptCallback>> m_acceptCallbackQueue;
	MT::Deque<std::pair<size_t, ParserCallback>> m_parserCallbackQueue;
	MT::Deque<std::pair<SessionData, SessionCallback>> m_sessionCallbackQueue;

	size_t m_threadCount;

public:

	IOContext(size_t threadCount = std::thread::hardware_concurrency() * 4) :
		m_threadCount(threadCount) {
		m_pool.init(m_threadCount);
	};

	void run() {
		m_shouldRun = true;
		std::pair<Network::Socket, AcceptCallback> bufferAccept;
		std::pair<size_t, ParserCallback> bufferParser;
		std::pair<SessionData, SessionCallback> bufferSession;

		while (m_shouldRun.load())
		{
			if (m_acceptCallbackQueue.waitAndPopBackFor(bufferAccept, std::chrono::milliseconds(50)))
				bufferAccept.second(std::move(bufferAccept.first));
			if (m_parserCallbackQueue.waitAndPopBackFor(bufferParser, std::chrono::milliseconds(50)))
				bufferParser.second(std::move(bufferParser.first));
			if (m_sessionCallbackQueue.waitAndPopBackFor(bufferSession, std::chrono::milliseconds(50)))
				bufferSession.second(std::move(bufferSession.first));
		}
	}

	void stop() {
		m_shouldRun = false;
		m_pool.shutdown();
	}

	void post(std::function<void()> task) {
		m_pool.pushTask(task);
	}

	void postAcceptCallback (Network::Socket&& socket, AcceptCallback task) {
		m_acceptCallbackQueue.pushBack(std::make_pair(std::move(socket), task));
	}

	void postParserCallback(size_t bytesRead, ParserCallback task) {
		m_parserCallbackQueue.pushBack(std::make_pair(bytesRead, task));
	}

	void postSessionCallback(SessionData&& data, SessionCallback task) {
		m_sessionCallbackQueue.pushBack(std::make_pair(std::move(data), task));
	}

	~IOContext() {
	}

};

