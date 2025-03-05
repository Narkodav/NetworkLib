#pragma once
#include "Common.h"

class Socket {
public:

    enum class Error {
        NONE,
        WOULD_BLOCK,     // WSAEWOULDBLOCK / EAGAIN / EWOULDBLOCK
        INTERRUPTED,     // WSAEINTR / EINTR
        DISCONNECTED,    // Connection closed
        CONN_REFUSED,    // WSAECONNREFUSED / ECONNREFUSED
        CONN_RESET,      // WSAECONNRESET / ECONNRESET
        CONN_ABORTED,    // WSAECONNABORTED / ECONNABORTED
        TIMED_OUT,       // WSAETIMEDOUT / ETIMEDOUT
        IN_PROGRESS,     // WSAEINPROGRESS / EINPROGRESS
        ALREADY,         // WSAEALREADY / EALREADY
        NOT_CONNECTED,   // WSAENOTCONN / ENOTCONN
        ADDR_IN_USE,     // WSAEADDRINUSE / EADDRINUSE
        ADDR_NOT_AVAIL,  // WSAEADDRNOTAVAIL / EADDRNOTAVAIL
        NET_UNREACHABLE, // WSAENETUNREACH / ENETUNREACH
        HOST_UNREACHABLE,// WSAEHOSTUNREACH / EHOSTUNREACH
        INVALID_ARG,     // WSAEINVAL / EINVAL
        UNKNOWN          // Other errors
    };

    static inline const std::map<Error, std::string> errorStrings = {
    {Error::NONE, "No error"},
    {Error::WOULD_BLOCK, "Operation would block"},
    {Error::INTERRUPTED, "Operation interrupted"},
    {Error::DISCONNECTED, "Connection disconnected"},
    {Error::CONN_REFUSED, "Connection refused by peer"},
    {Error::CONN_RESET, "Connection reset by peer"},
    {Error::CONN_ABORTED, "Connection aborted"},
    {Error::TIMED_OUT, "Operation timed out"},
    {Error::IN_PROGRESS, "Operation in progress"},
    {Error::ALREADY, "Operation already in progress"},
    {Error::NOT_CONNECTED, "Socket not connected"},
    {Error::ADDR_IN_USE, "Address already in use"},
    {Error::ADDR_NOT_AVAIL, "Address not available"},
    {Error::NET_UNREACHABLE, "Network is unreachable"},
    {Error::HOST_UNREACHABLE, "Host is unreachable"},
    {Error::INVALID_ARG, "Invalid argument"},
    {Error::UNKNOWN, "Unknown error"}
    };

#ifdef _WIN32
    static inline const std::map<int, Error> errorMap = {
        {0, Error::NONE},
        {WSAEWOULDBLOCK, Error::WOULD_BLOCK},
        {WSAEINTR, Error::INTERRUPTED},
        {WSAECONNREFUSED, Error::CONN_REFUSED},
        {WSAECONNRESET, Error::CONN_RESET},
        {WSAECONNABORTED, Error::CONN_ABORTED},
        {WSAETIMEDOUT, Error::TIMED_OUT},
        {WSAEINPROGRESS, Error::IN_PROGRESS},
        {WSAEALREADY, Error::ALREADY},
        {WSAENOTCONN, Error::NOT_CONNECTED},
        {WSAEADDRINUSE, Error::ADDR_IN_USE},
        {WSAEADDRNOTAVAIL, Error::ADDR_NOT_AVAIL},
        {WSAENETUNREACH, Error::NET_UNREACHABLE},
        {WSAEHOSTUNREACH, Error::HOST_UNREACHABLE},
        {WSAEINVAL, Error::INVALID_ARG}
    };
#else
    static inline const std::map<int, Error> errorMap = {
        {0, Error::NONE},
        {EAGAIN, Error::WOULD_BLOCK},
        {EWOULDBLOCK, Error::WOULD_BLOCK},
        {EINTR, Error::INTERRUPTED},
        {ECONNREFUSED, Error::CONN_REFUSED},
        {ECONNRESET, Error::CONN_RESET},
        {ECONNABORTED, Error::CONN_ABORTED},
        {ETIMEDOUT, Error::TIMED_OUT},
        {EINPROGRESS, Error::IN_PROGRESS},
        {EALREADY, Error::ALREADY},
        {ENOTCONN, Error::NOT_CONNECTED},
        {EADDRINUSE, Error::ADDR_IN_USE},
        {EADDRNOTAVAIL, Error::ADDR_NOT_AVAIL},
        {ENETUNREACH, Error::NET_UNREACHABLE},
        {EHOSTUNREACH, Error::HOST_UNREACHABLE},
        {EINVAL, Error::INVALID_ARG}
    };
#endif
    static constexpr int DEFAULT_BACKLOG = 4096;

private:
    int m_timeout = 0;  // in seconds
    bool m_nonBlocking = false;

#ifdef _WIN32
    SOCKET m_sockfd;
#else
    int m_sockfd;
#endif
    int m_domain;
    int m_type;
    int m_protocol;
    sockaddr_in m_addr;

    bool m_isConnected = false;

protected:
    // Constructor for accepted sockets
#ifdef _WIN32
    Socket(SOCKET fd, sockaddr_in adr,
        int timeout = 30, bool nonBlocking = false) :
        m_sockfd(fd), m_addr(adr), m_isConnected(true),
        m_timeout(0), m_nonBlocking(false) {};

#else
    Socket(int fd, sockaddr_in adr) :
        m_sockfd(fd), m_addr(adr), m_isConnected(true),
        m_timeout(s_commTimeout), m_nonBlocking(s_commNonBlocking) {
        setTimeout(m_timeout);
        setNonBlocking(m_nonBlocking);
    };

#endif

public:
    Socket(int domain = AF_INET, int type = SOCK_STREAM, int protocol = 0) :
        m_domain(domain), m_type(type), m_protocol(protocol),
        m_timeout(0), m_nonBlocking(false) {
        m_sockfd = socket(domain, type, protocol);

        if (m_sockfd < 0) {
            throw std::runtime_error("Error creating a socket: " +
                getLastErrorString());
        }
    }

    ~Socket() {
        close();
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept
    {
        m_sockfd = std::exchange(other.m_sockfd, -1);
        m_domain = std::exchange(other.m_domain, -1);
        m_type = std::exchange(other.m_type, -1);
        m_protocol = std::exchange(other.m_protocol, -1);
        m_addr = std::exchange(other.m_addr, {});
        m_isConnected = std::exchange(other.m_isConnected, false);
    }

    // Move assignment
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();  // Close existing socket if any
            m_sockfd = std::exchange(other.m_sockfd, -1);
            m_domain = std::exchange(other.m_domain, -1);
            m_type = std::exchange(other.m_type, -1);
            m_protocol = std::exchange(other.m_protocol, -1);
            m_addr = std::exchange(other.m_addr, {});
            m_isConnected = std::exchange(other.m_isConnected, false);
        }
        return *this;
    }

    void bind(uint16_t port, const char* ip = nullptr) {
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

    void listen(int backlog = DEFAULT_BACKLOG) {
        if (::listen(m_sockfd, backlog) < 0) {
            throw std::runtime_error("Listen failed: " +
                getLastErrorString());
        }
    }

    Socket accept() {
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

    void connect(const char* ip, uint16_t port) {
        std::vector<char> binaryIp(4);
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_pton(AF_INET, ip, binaryIp.data());

        if (::connect(m_sockfd, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
            throw std::runtime_error("Connect failed" +
                getLastErrorString());
        }
        m_isConnected = true;
    }

    size_t send(const char* data, size_t len) {
#ifdef _WIN32
        return ::send(m_sockfd, data, len, 0);
#else
        return ::write(m_sockfd, data, len);
#endif
    }

    int receive(char* buffer, size_t len) {
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

    void close() {
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

    Socket& setTimeout(int seconds) {
        m_timeout = seconds;
#ifdef _WIN32
        DWORD timeout = m_timeout * 1000;
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) != 0) {
            throw std::runtime_error("Failed to set socket timeout");
        }
#else
        struct timeval timeout;
        timeout.tv_sec = m_timeout;
        timeout.tv_usec = 0;
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
            throw std::runtime_error("Failed to set socket timeout");
        }
#endif
        return *this;
    }

    Socket& setNonBlocking(bool nonBlocking = true) {
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

    static Error getLastError() {
#ifdef _WIN32
        int error = WSAGetLastError();
#else
        int error = errno;
#endif
        auto it = errorMap.find(error);
        return (it != errorMap.end()) ? it->second : Error::UNKNOWN;
    }

    static std::string getErrorString(Error error) {
        auto it = errorStrings.find(error);
        return (it != errorStrings.end()) ? it->second : "Undefined error";
    }

    static std::string getLastErrorString() {
        return getErrorString(getLastError());
    }
};

