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
		auto& body = message->getBody();
		if(body != nullptr)
		{
			auto& headers = message->getHeaders();
			auto size = headers.get(Message::Headers::Standard::CONTENT_LENGTH);
			if (size != "")
				return body->sendTransferSize(sock, std::stoi(size), MAX_RETRY_COUNT, MAX_BODY_SIZE);
			else if (headers.get(Message::Headers::Standard::TRANSFER_ENCODING) == "Chunked")
				return body->sendChunked(sock, MAX_RETRY_COUNT, MAX_BODY_SIZE);
			else throw std::runtime_error("No transfer method specified for the body");
		}
	}

	size_t Sender::send(Socket& sock, MessagePtr& message)
	{
		if (message == nullptr)
			throw std::runtime_error("trying to send empty message");
		size_t bytesSent = 0;
		bytesSent += sendHeaders(sock, message);
		bytesSent += sendBody(sock, message);
		return bytesSent;
	}

}