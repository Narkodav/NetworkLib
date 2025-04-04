# NetworkLib

A C++ networking library providing abstractions for socket programming and HTTP server implementation.

## Overview

NetworkLib is a modular C++ library that simplifies network programming by providing high-level abstractions for common networking tasks. It includes socket management, session handling, and HTTP server implementation.

## Core Components

### Socket Abstraction
- Wraps platform-specific socket implementations
- Handles connection management
- Provides synchronous I/O operations
- Error handling and status reporting

### Sender and Receiver
- Manages data transmission and reception
- Buffer management for efficient data handling
- Support for different transmission modes (chunked, fixed-size)
- Error recovery and retry mechanisms

### Session Management
- Handles client connections
- Maintains connection state
- Implements timeout handling
- Tracks session statistics (bytes sent/received, requests handled)

### HTTP Server
- Simple HTTP server implementation
- Request parsing and routing
- Response generation
- Support for common HTTP methods
- Static file serving capabilities

## Dependencies

### CommonApi
This library requires the thread pool implementation from CommonApi. Make sure to include CommonApi in your project before using NetworkLib.

## Usage Examples

```cpp
#include "Server.h"

int main() {
	IOContext ioContext;
	http::Server server(ioContext, 8080);
	
	server.startBlocking();
}

//basic server implementation
#include "Socket.h"

int main() {
	Socket sock;

	sock.connect("127.0.0.1", 8080);

	std::fstream file("public/cat.jpg", std::ios::in | std::ios::out | std::ios::app | std::ios::binary);

	std::vector<char> buffer(256);
	size_t bytesRead = file.read(buffer.data(), buffer.size()).gcount();
	if (bytesRead < buffer.size())
		return 0;

	sock.sendLoop(buffer.data(), buffer.size(), 0, 5,
		[&buffer, &file](char*& buf, size_t& len, size_t bytesSent, size_t& receivedTotal) {
			for (size_t i = bytesSent; i < len; i++)
				buffer[i - bytesSent] = buffer[i];

			size_t bytesLeft = len - bytesSent;
			size_t bytesRead = file.read(buffer.data() + bytesLeft, buffer.size() - bytesLeft).gcount();

			len -= bytesSent - bytesRead;
			if (len > 0)
				return true;
			return false;
		});
}

//basic client implementation
#include "Socket.h"

int main() {
	Socket sock;

	sock.bind(8080);
	sock.listen();

	Socket client = sock.accept();

	client.waitForData(std::chrono::seconds(5));

	std::vector<char> msg;
	msg.resize(256);

    client.receiveLoop(msg.data(),
        msg.size(), 0, 5,
        [&msg]
        (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {
            for (int i = 0; i < bytesRead; ++i)
                std::cout << buffer[i];
            return true;
        }
    );

    return 0;
}
