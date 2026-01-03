#include "include/RestfulServer.h"
#define assert(x) if(!(x)) throw std::runtime_error("Assertion failed: " #x);

#include "TaskManager.h"

int main()
{
	Network::HTTP::RestfulServer server(8080, "RestfulServer");
	TaskManager taskManager;
	taskManager.registerRoutes(server);
	server.start();
	return 0;
}