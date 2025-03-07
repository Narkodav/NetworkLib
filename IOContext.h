#pragma once
#include "Common.h"
#include "Socket.h"


class IOContext
{
public:
	using AcceptCallback = std::function<void(Socket&&)>;
	using ParserCallback = std::function<void(size_t)>;

private:
	MT::ThreadPool m_pool;
	std::atomic<bool> m_shouldRun;
	MT::Deque<std::pair<Socket, AcceptCallback>> m_acceptCallbackQueue;
	MT::Deque<std::pair<size_t, ParserCallback>> m_parserCallbackQueue;
	size_t m_threadCount;

public:

	IOContext(size_t threadCount = std::thread::hardware_concurrency() * 4) :
		m_threadCount(threadCount) {
		m_pool.init(m_threadCount);
	};

	void run() {
		m_shouldRun = true;
		std::pair<Socket, AcceptCallback> bufferAccept;
		std::pair<size_t, ParserCallback> bufferParser;

		while (m_shouldRun.load())
		{
			if (m_acceptCallbackQueue.waitAndPopBackFor(bufferAccept, std::chrono::milliseconds(100)))
				bufferAccept.second(std::move(bufferAccept.first));
			if (m_parserCallbackQueue.waitAndPopBackFor(bufferParser, std::chrono::milliseconds(100)))
				bufferParser.second(std::move(bufferParser.first));

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

	void postAcceptCallback (Socket&& socket, AcceptCallback task) {
		m_acceptCallbackQueue.pushBack(std::make_pair(std::move(socket), task));
	}

	void postParserCallback(size_t bytesRead, ParserCallback task) {
		m_parserCallbackQueue.pushBack(std::make_pair(bytesRead, task));
	}

	~IOContext() {
	}

};

