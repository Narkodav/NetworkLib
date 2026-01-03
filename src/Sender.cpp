#include "../include/Sender.h"

namespace Network::HTTP
{

	size_t Sender::sendHeaders(Socket& sock, std::unique_ptr<Message>& message)
	{
		std::string headers = message->getFirstLine();
		for (const auto& [header, value] : message->getHeaders()) {
			headers += header + ": " + value + "\r\n";
		}
		headers += "\r\n";
		return sock.sendCommited(headers.data(), headers.size(), s_maxRetryCount);
	}

	size_t Sender::sendBody(Socket& sock, std::unique_ptr<Message>& message)
	{
		auto& body = message->getBody();
		if(body != nullptr)
		{
			auto& headers = message->getHeaders();
			auto size = headers.get(Message::Headers::Standard::ContentLength);
			if (size != "")
				return body->sendTransferSize(sock, std::stoi(size), s_maxRetryCount, s_maxBodySize);
			else if (headers.get(Message::Headers::Standard::TransferEncoding) == "Chunked")
				return body->sendChunked(sock, s_maxRetryCount, s_maxBodySize);
			else throw std::runtime_error("No transfer method specified for the body");
		}
	}

	size_t Sender::send(Socket& sock, std::unique_ptr<Message>& message)
	{
		if (message == nullptr)
			throw std::runtime_error("trying to send empty message");
		size_t bytesSent = 0;
		bytesSent += sendHeaders(sock, message);
		bytesSent += sendBody(sock, message);
		return bytesSent;
	}

}