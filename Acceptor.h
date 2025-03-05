#pragma once
#include "Common.h"
#include "Socket.h"
#include "IOContext.h"

// acceptor itself is not thread safe
// only the acceptor can make sockets
class Acceptor
{
private:
	Socket m_acceptSocket;

	IOContext& m_acceptIOContext;
	int m_port;


public:
	Acceptor(IOContext& context, int port) : 
	m_acceptIOContext(context), m_port(port), m_acceptSocket()
	{
		m_acceptSocket.bind(port);
		m_acceptSocket.listen();
	}
	~Acceptor() {
		m_acceptSocket.close();
	};

	void asyncAccept(std::function<void(Socket&&)> acceptCallback,
		int clientTimeout = 30, bool clientNonBlocking = true)
	{
		m_acceptIOContext.post([this,
			callback = std::move(acceptCallback),
			clientTimeout = clientTimeout,
			clientNonBlocking = clientNonBlocking]() {
				try {
					Socket client(std::move(m_acceptSocket.accept()));
					client
						.setNonBlocking(clientNonBlocking)
						.setTimeout(clientTimeout);
					m_acceptIOContext.postAcceptCallback(std::move(client), callback);
				}
				catch (const std::exception& e) {
					// If accept failed, repost the accept operation
					asyncAccept(std::move(callback), clientTimeout, clientNonBlocking);

					// Optionally log the error
					 std::cerr << "Accept error: " << e.what() << std::endl;
				}
			});
		
	}





};

