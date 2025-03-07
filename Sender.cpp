#include "Sender.h"

namespace http
{

	size_t Sender::sendHeaders(Socket& sock, MessagePtr& message)
	{
		std::string headers = message->getFirstLine();
		for (const auto& [header, value] : message->getHeaders()) {
			headers += header + ": " + value + "\r\n";
		}
		headers += "\r\n";

		return sock.sendCommited(headers.data(), headers.size(), MAX_RETRY_COUNT);
	}

	size_t Sender::sendBody(Socket& sock, MessagePtr& message)
	{


	}

	size_t Sender::send(Socket& sock, MessagePtr& message)
	{


	}

}