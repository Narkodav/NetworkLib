#include "Acceptor.h"
#include "IOContext.h"
#include "Parser.h"

void acceptCallback(Socket&& client, Acceptor& acceptor)
{
	acceptor.asyncAccept([&acceptor](Socket&& client) {
		acceptCallback(std::move(client), acceptor);
		});

	std::cout << "client connected" << std::endl;
	std::array<char, 1024> buffer;
	http::Parser::read(client, buffer);
	client.close();
}


int main()
{
	IOContext ioContext;
	Acceptor acceptor(ioContext, 8080);
	
	acceptor.asyncAccept([&acceptor](Socket&& client) {
		
		});

	ioContext.run();
	return 0;
}