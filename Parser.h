#pragma once
#include "Socket.h"
#include "IOContext.h"

// takes a client socket a buffer and a io context returns an http request
// doesn't actually store anything or have a state so its static

class HTTPParser
{
	static inline const size_t MAX_RETRY_COUNT = 5;

public:
	template <size_t bufferSize>
    static void assyncRead(Socket&& client, std::array<char, bufferSize>&& buffer,
        IOContext& ioContext, std::function<void()> callback)
    {
        ioContext.post([&, sock = std::move(client), buf = std::move(buffer)]() {
            read(sock, buf);
            });

    };

    template <size_t bufferSize>
    static void read(Socket& client, std::array<char, bufferSize>& buffer)
    {
        int bytesRead = 0;
        int retryCount = 0;

        while (true) {
            bytesRead = client.receive(buffer.data(), bufferSize);

            if (bytesRead > 0) {
                // Reset retry count on successful read
                retryCount = 0;
                std::cout.write(buffer.data(), bytesRead);
                std::cout.flush();
                std::fill(buffer.begin(), buffer.end(), 0);
            }
            else if (bytesRead == 0) {
                std::cout << "Connection closed by client" << std::endl;
                break;
            }
            else if (bytesRead < 0) {
                auto error = Socket::getLastError();
                if (error == Socket::Error::INTERRUPTED ||
                error == Socket::Error::WOULD_BLOCK) {
                    if (++retryCount > MAX_RETRY_COUNT) {
                        std::cerr << "Max retries exceeded" << std::endl;
                        break;
                    }
                    // Exponential backoff: 10ms, 20ms, 40ms, 80ms, 160ms
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(10 * (1 << (retryCount - 1)))
                    );
                    continue;
                }
                else {
                    std::cerr << "Error reading from socket: " << Socket::getErrorString(error) << std::endl;
                    break;
                }
            }
        }
    };
};

