#include "Acceptor.h"
#include "IOContext.h"
#include "Parser.h"

int main()
{
	IOContext ioContext;
	Acceptor acceptor(ioContext, 8080);
	
	acceptor.asyncAccept([&ioContext](Socket&& client) {
		std::cout << "client connected" << std::endl;
		std::array<char, 1024> buffer;

		HTTPParser::read(client, buffer);
		ioContext.stop();
		});

	ioContext.run();
	return 0;
}