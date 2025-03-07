#include "Server.h"

namespace http
{
	void Server::startBlocking()
	{
		try
		{
			accept();
			m_context.run();
		}
		catch (std::exception e)
		{
			throw std::runtime_error("Server failed to start");
		}
	}

	void Server::accept()
	{
		m_acceptor.asyncAccept([this](Socket&& socket) {
			accept();
			processMessage(std::move(socket));
			});
	}

	void Server::processMessage(Socket&& socket)
	{
		auto socket_ptr = std::make_shared<Socket>(std::move(socket));
		auto leftovers_ptr = std::make_shared<std::string>();
		auto message_ptr = std::make_shared<MessagePtr>();

		Parser::asyncReadHeader(m_context, *socket_ptr, *leftovers_ptr, *message_ptr,
			[this, socket_ptr, leftovers_ptr, message_ptr](size_t bytesTransferred) {



			});

	}
}