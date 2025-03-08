#include "Server.h"

int main()
{
	IOContext ioContext;
	http::Server server(ioContext, 8080);
	
	server.startBlocking();

	return 0;
}