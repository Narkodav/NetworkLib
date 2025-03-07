#include "Socket.h"

void Socket::bind(uint16_t port, const char* ip = nullptr) {
    std::vector<char> binaryIp(4);
    m_addr.sin_family = m_domain;
    m_addr.sin_port = htons(port);
    if (ip == nullptr) {
        m_addr.sin_addr.s_addr = INADDR_ANY;
    }
    else {
        if (inet_pton(AF_INET, ip, &m_addr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid IP address");
        }
    }

    if (::bind(m_sockfd, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
        throw std::runtime_error("Bind failed: " +
            getLastErrorString());
    }
}

void Socket::listen(int backlog = DEFAULT_BACKLOG) {
    if (::listen(m_sockfd, backlog) < 0) {
        throw std::runtime_error("Listen failed: " +
            getLastErrorString());
    }
}

Socket Socket::accept() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

#ifdef _WIN32
    SOCKET clientfd = ::accept(m_sockfd, (struct sockaddr*)&client_addr, &client_len);
#else
    int clientfd = ::accept(m_sockfd, (struct sockaddr*)&client_addr, &client_len);
#endif

    if (clientfd < 0) {
        throw std::runtime_error("Accept failed: " +
            getLastErrorString());
    }

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, INET_ADDRSTRLEN);
    m_isConnected = true;
    Socket client(clientfd, client_addr);
    return client;
}

void Socket::connect(const char* ip, uint16_t port) {
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &m_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid IP address");
    }

    if (::connect(m_sockfd, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
        throw std::runtime_error("Connect failed" +
            getLastErrorString());
    }
    m_isConnected = true;
}

int Socket::send(const char* data, size_t len) {
#ifdef _WIN32
    return ::send(m_sockfd, data, len, 0);
#else
    return ::write(m_sockfd, data, len);
#endif
}

int Socket::sendCommited(const char* data, size_t len, size_t maxRetryCount)
{
    if (m_sockfd < 0) {
        throw std::runtime_error("Client socket is not connected: " +
            getLastErrorString());
    }
    size_t retryCount = 0;
    size_t sentTotal = 0;
    int bytesSent = 0;

    while (sentTotal < len) {

#ifdef _WIN32
        bytesSent = ::send(m_sockfd, data + sentTotal, len - sentTotal, 0);
#else
        bytesSent = ::write(m_sockfd, data + sentTotal, len - sentTotal);
#endif
        if (bytesSent > 0) {
            // Reset retry count on successful send
            retryCount = 0;
            sentTotal += bytesSent;
        }
        else if (bytesSent == 0) {
            // For send(), 0 indicates an error condition
            throw std::runtime_error("Connection closed unexpectedly");
        }
        else if (bytesSent < 0) {
            auto error = Socket::getLastError();
            if (error == Socket::Error::INTERRUPTED ||
                error == Socket::Error::WOULD_BLOCK) {
                if (++retryCount > maxRetryCount) {
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
                std::cerr << "Error sending to socket: " << Socket::getErrorString(error) << std::endl;
                break;
            }
        }
    }
    return sentTotal;
}

int Socket::sendLoop(char* buffer, size_t len, size_t totalStart, size_t maxRetryCount,
    std::function<bool(char*&, size_t&, size_t, size_t&)> handler)
{
    if (m_sockfd < 0) {
        throw std::runtime_error("Client socket is not connected: " +
            getLastErrorString());
    }
    size_t retryCount = 0;
    int bytesSent = 0;
    bool shouldSend = true;

    while (shouldSend) {

#ifdef _WIN32
        bytesSent = ::send(m_sockfd, buffer, len, 0);
#else
        bytesSent = ::write(m_sockfd, buffer, len);
#endif
        if (bytesSent > 0) {
            // Reset retry count on successful send
            retryCount = 0;
            totalStart += bytesSent;
            shouldSend = handler(buffer, len, bytesSent, totalStart);
        }
        else if (bytesSent == 0) {
            // For send(), 0 indicates an error condition
            throw std::runtime_error("Connection closed unexpectedly");
        }
        else if (bytesSent < 0) {
            auto error = Socket::getLastError();
            if (error == Socket::Error::INTERRUPTED ||
                error == Socket::Error::WOULD_BLOCK) {
                if (++retryCount > maxRetryCount) {
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
                std::cerr << "Error sending to socket: " << Socket::getErrorString(error) << std::endl;
                break;
            }
        }
    }
    return totalStart;
}

int Socket::receive(char* buffer, size_t len) {
    if (m_sockfd < 0) {
        throw std::runtime_error("Client socket is not connected: " +
            getLastErrorString());
    }
#ifdef _WIN32
    return ::recv(m_sockfd, buffer, len, 0);
#else
    return ::read(m_sockfd, buffer, len);
#endif
}

//returns available data size
u_long Socket::checkDataAvailable()
{
    if (m_sockfd < 0) {
        throw std::runtime_error("Client socket is not connected: " +
            getLastErrorString());
    }

#ifdef _WIN32
    u_long bytesAvailable = 0;
    if (ioctlsocket(m_sockfd, FIONREAD, &bytesAvailable) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to check available data");
    }
#else
    int bytesAvailable = 0;
    if (ioctl(m_sockfd, FIONREAD, &bytesAvailable) == -1) {
        throw std::runtime_error("Failed to check available data: " +
            getLastErrorString());
    }
#endif
    return bytesAvailable;
}

int Socket::receiveLoop(char* buffer, size_t len, size_t totalStart, size_t maxRetryCount,
    std::function<bool(char*&, size_t&, size_t, size_t&)> handler) {
    if (m_sockfd < 0) {
        throw std::runtime_error("Client socket is not connected: " +
            getLastErrorString());
    }
    size_t retryCount = 0;
    bool shouldRead = true;

    while (shouldRead) {

#ifdef _WIN32
        auto bytesRead = ::recv(m_sockfd, buffer, len, 0);
#else
        auto bytesRead = ::read(m_sockfd, buffer, len);
#endif
        if (bytesRead > 0) {
            // Reset retry count on successful read
            retryCount = 0;
            totalStart += bytesRead;
            shouldRead = handler(buffer, len, bytesRead, totalStart);
        }
        else if (bytesRead == 0) {
            std::cout << "Connection closed by client" << std::endl;
            break;
        }
        else if (bytesRead < 0) {
            auto error = Socket::getLastError();
            if (error == Socket::Error::INTERRUPTED ||
                error == Socket::Error::WOULD_BLOCK) {
                if (++retryCount > maxRetryCount) {
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
    return totalStart;
}

void Socket::close() {
    if (m_sockfd >= 0) {
        // Shutdown gracefully first
        shutdown(m_sockfd,
#ifdef _WIN32
            SD_BOTH
#else
            SHUT_RDWR
#endif
        );

#ifdef _WIN32
        closesocket(m_sockfd);
#else
        ::close(m_sockfd);
#endif
        m_sockfd = -1;  // Mark as closed
        m_isConnected = false;
    }
}

Socket& Socket::setNonBlocking(bool nonBlocking = true) {
    m_nonBlocking = nonBlocking;
#ifdef _WIN32
    u_long mode = m_nonBlocking ? 1 : 0;
    if (ioctlsocket(m_sockfd, FIONBIO, &mode) != 0) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
#else
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(m_sockfd, F_SETFL, m_nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
#endif
    return *this;
}

int Socket::getReceiveBufferSize() {
    if (m_sockfd < 0) {
        throw std::runtime_error("Socket is not connected: " +
            getLastErrorString());
    }

    int size = 0;
    socklen_t optlen = sizeof(size);

#ifdef _WIN32
    if (getsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF,
        reinterpret_cast<char*>(&size), &optlen) == SOCKET_ERROR) {
#else
    if (getsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF,
        &size, &optlen) < 0) {
#endif
        throw std::runtime_error("Failed to get buffer size: " +
            getLastErrorString());
    }
    return size;
}

void Socket::setReceiveBufferSize(int size) {
    if (m_sockfd < 0) {
        throw std::runtime_error("Socket is not connected: " +
            getLastErrorString());
    }

#ifdef _WIN32
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF,
        reinterpret_cast<const char*>(&size), sizeof(size)) == SOCKET_ERROR) {
#else
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF,
        &size, sizeof(size)) < 0) {
#endif
        throw std::runtime_error("Failed to set buffer size: " +
            getLastErrorString());
    }
}

Socket::Error Socket::getLastError() {
#ifdef _WIN32
    int error = WSAGetLastError();
#else
    int error = errno;
#endif
    auto it = errorMap.find(error);
    return (it != errorMap.end()) ? it->second : Error::UNKNOWN;
}

std::string Socket::getErrorString(Error error) {
    auto it = errorStrings.find(error);
    return (it != errorStrings.end()) ? it->second : "Undefined error";
}

std::string Socket::getLastErrorString() {
    return getErrorString(getLastError());
}