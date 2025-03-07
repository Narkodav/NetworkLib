#include "Acceptor.h"
#include "IOContext.h"
#include "Parser.h"

void acceptCallback(Socket&& client, Acceptor& acceptor, IOContext& context)
{
	//acceptor.asyncAccept([&acceptor](Socket&& client) {
	//	acceptCallback(std::move(client), acceptor);
	//	});

	std::cout << "client connected" << std::endl;
	http::Parser parser;
	auto message = parser.parseMessage(client);
	client.close();
	
	for (const auto& [name, value] : message->getHeaders()) {
		std::cout << name << ": " << value << std::endl;
	}

	context.stop();
}


int main()
{
	IOContext ioContext;
	Acceptor acceptor(ioContext, 8080);
	
	acceptor.asyncAccept([&acceptor, &ioContext](Socket&& client) {
		acceptCallback(std::move(client), acceptor, ioContext);
		});

	ioContext.run();

	//http::Message::Headers headers;
	//headers.set(http::Message::Headers::Standard::CONTENT_TYPE, "text/plain");
	//headers.set("X-Custom-Header", "custom value");
	//headers.set(http::Message::Headers::Standard::HOST, "example.com");

	//for (const auto& [name, value] : headers) {
	//	std::cout << name << ": " << value << std::endl;
	//}

	return 0;
}