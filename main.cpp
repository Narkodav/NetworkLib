#include "Server.h"
#include "Session.h"

int main()
{

	//Acceptor acceptor(ioContext, 8080);

	//acceptor.asyncAccept([](Socket&& sock)
	//	{
	//		auto session = std::make_shared<http::Session>(
	//			std::move(sock), 
	//			[](std::unique_ptr<http::Message>& msg) {
	//				return std::make_unique<http::StringBody>();
	//			},
	//			[](std::unique_ptr<http::Message>& msg) {
	//				return nullptr;
	//			});

	//		session->start();

	//	}
	//
	//);

	//ioContext.run();

	IOContext ioContext;
	http::Server server(ioContext, 8080);
	
	server.startBlocking();

	//Socket sock;

	//sock.connect("127.0.0.1", 8080);

	//std::fstream file("public/cat.jpg", std::ios::in | std::ios::out | std::ios::app | std::ios::binary);

	//std::vector<char> buffer(256);
	//size_t bytesRead = file.read(buffer.data(), buffer.size()).gcount();
	//if (bytesRead < buffer.size())
	//	return 0;

	//sock.sendLoop(buffer.data(), buffer.size(), 0, 5,
	//	[&buffer, &file](char*& buf, size_t& len, size_t bytesSent, size_t& receivedTotal) {
	//		for (size_t i = bytesSent; i < len; i++)
	//			buffer[i - bytesSent] = buffer[i];

	//		size_t bytesLeft = len - bytesSent;
	//		size_t bytesRead = file.read(buffer.data() + bytesLeft, buffer.size() - bytesLeft).gcount();

	//		len -= bytesSent - bytesRead;
	//		if (len > 0)
	//			return true;
	//		return false;
	//	});

	return 0;
}