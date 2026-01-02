#include "../include/Socket.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace Network {
    static_assert(sizeof(Socket::AddressIn) == sizeof(sockaddr_in), "SocketAddressIn size mismatch");

#ifdef _WIN32
    static_assert(sizeof(Socket::Handle) == sizeof(SOCKET), "SocketHandle size mismatch");
#else
    static_assert(sizeof(Socket::Handle) == sizeof(int), "SocketHandle size mismatch");
#endif

    // pretty much all networking uses Socket abstraction
#ifdef _WIN32
    class WSAInitializer {
    private:
        WSAInitializer() {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                throw std::runtime_error("WSAStartup failed");
            }

            if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
                WSACleanup();
                throw std::runtime_error("Could not find Winsock 2.2");
            }
        }

    public:
        WSAInitializer(WSAInitializer const&) = delete;
        void operator=(WSAInitializer const&) = delete;

        static WSAInitializer& instance() {
            try {
                static WSAInitializer inst;
                return inst;
            }
            catch (const std::exception& e) {
#ifndef MANUAL_WSA_INIT
                std::cerr << "Failed to initialize WSA: " << e.what() << std::endl;
                std::exit(1);
#else
                throw std::runtime_error("Failed to initialize WSA: " + std::string(e.what()));
#endif
            }
        }

        ~WSAInitializer() {
            WSACleanup();
        }
    };

#ifndef MANUAL_WSA_INIT //define this if you want to manually init the WSA or have custom init error handling
    static auto& WSAinit = WSAInitializer::instance();
#endif
#endif

    Socket::Socket(Domain domain //= Domain::IPv4
        , Type type //= Type::Stream
        , Protocol protocol //= Protocol::Default
    ) :
        m_domain(domain), m_type(type), m_protocol(protocol),
        m_timeout(0), m_nonBlocking(false) {
        m_sockfd = socket(static_cast<int>(domain), static_cast<int>(type), static_cast<int>(protocol));

        if (m_sockfd < 0) {
            throw std::runtime_error("Error creating a socket: " +
                getLastErrorString());
        }
    }

    void Socket::bind(uint16_t port, const char* ip /*= nullptr*/) {
        std::vector<char> binaryIp(4);
        m_addr.sin_family = static_cast<int>(m_domain);
        m_addr.sin_port = htons(port);
        if (ip == nullptr) {
            reinterpret_cast<sockaddr_in&>(m_addr).sin_addr.s_addr = INADDR_ANY;
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

    void Socket::listen(int backlog /*= DEFAULT_BACKLOG*/) {
        if (::listen(m_sockfd, backlog) < 0) {
            throw std::runtime_error("Listen failed: " +
                getLastErrorString());
        }
    }

    Socket Socket::accept() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        Handle clientfd = ::accept(m_sockfd, (struct sockaddr*)&client_addr, &client_len);

        if (clientfd < 0) {
            throw std::runtime_error("Accept failed: " +
                getLastErrorString());
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, INET_ADDRSTRLEN);
        m_isConnected = true;
        Socket client(clientfd, reinterpret_cast<AddressIn&>(client_addr));
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
                if (error == Socket::Error::Interrupted ||
                    error == Socket::Error::WouldBlock) {
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
                if (error == Socket::Error::Interrupted ||
                    error == Socket::Error::WouldBlock) {
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
    uint32_t Socket::checkDataAvailable()
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
                if (error == Socket::Error::Interrupted ||
                    error == Socket::Error::WouldBlock) {
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

    Socket& Socket::setNonBlocking(bool nonBlocking /*= true*/) {
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
        return static_cast<Error>(error);
    }

    std::string Socket::getErrorString(Error error) {
        auto it = s_errorStrings.find(error);
        return (it != s_errorStrings.end()) ? it->second : "Undefined error";
    }

    std::string Socket::getLastErrorString() {
        return getErrorString(getLastError());
    }

    bool Socket::waitForData(const std::chrono::seconds & secs, const std::chrono::microseconds & microsecs)
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

        if (bytesAvailable > 0)
            return true;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_sockfd, &readSet);

        timeval timeoutVal{
            static_cast<long>(secs.count()),
            static_cast<long>(microsecs.count())
        };

#ifdef _WIN32
        int result = select(0, &readSet, nullptr, nullptr, &timeoutVal);
#else
        // On Unix systems, first parameter is highest fd + 1
        int result = select(m_sockfd + 1, &readSet, nullptr, nullptr, &timeoutVal);
#endif

        if (result < 0) {
            if (getLastError() == Error::Interrupted)
                return false;  // Interrupted by signal
            throw std::runtime_error("Select failed: " + getLastErrorString());
        }

        return result > 0;  // True if data available
    }

    Socket& Socket::setTimeout(const std::chrono::seconds & timeout) {
        m_timeout = timeout.count();
#ifdef _WIN32
        DWORD timeoutMilisecs = m_timeout * 1000;
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMilisecs, sizeof(timeoutMilisecs)) != 0) {
            throw std::runtime_error("Failed to set socket timeout");
        }
#else
        struct timeval timeoutStruct;
        timeoutStruct.tv_sec = m_timeout;
        timeoutStruct.tv_usec = 0;
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeoutStruct, sizeof(timeoutStruct)) != 0) {
            throw std::runtime_error("Failed to set socket timeout");
        }
#endif
        return *this;
    }
}