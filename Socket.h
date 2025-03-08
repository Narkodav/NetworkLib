#pragma once
#include "Common.h"

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
    {Error::NONE,               "No error"},
    {Error::WOULD_BLOCK,        "Operation would block"},
    {Error::INTERRUPTED,        "Operation interrupted"},
    {Error::DISCONNECTED,       "Connection disconnected"},
    {Error::CONN_REFUSED,       "Connection refused by peer"},
    {Error::CONN_RESET,         "Connection reset by peer"},
    {Error::CONN_ABORTED,       "Connection aborted"},
    {Error::TIMED_OUT,          "Operation timed out"},
    {Error::IN_PROGRESS,        "Operation in progress"},
    {Error::ALREADY,            "Operation already in progress"},
    {Error::NOT_CONNECTED,      "Socket not connected"},
    {Error::ADDR_IN_USE,        "Address already in use"},
    {Error::ADDR_NOT_AVAIL,     "Address not available"},
    {Error::NET_UNREACHABLE,    "Network is unreachable"},
    {Error::HOST_UNREACHABLE,   "Host is unreachable"},
    {Error::INVALID_ARG,        "Invalid argument"},
    {Error::UNKNOWN,            "Unknown error"}
    };

#ifdef _WIN32
    static inline const std::map<int, Error> errorMap = {
        {0,                 Error::NONE},
        {WSAEWOULDBLOCK,    Error::WOULD_BLOCK},
        {WSAEINTR,          Error::INTERRUPTED},
        {WSAECONNREFUSED,   Error::CONN_REFUSED},
        {WSAECONNRESET,     Error::CONN_RESET},
        {WSAECONNABORTED,   Error::CONN_ABORTED},
        {WSAETIMEDOUT,      Error::TIMED_OUT},
        {WSAEINPROGRESS,    Error::IN_PROGRESS},
        {WSAEALREADY,       Error::ALREADY},
        {WSAENOTCONN,       Error::NOT_CONNECTED},
        {WSAEADDRINUSE,     Error::ADDR_IN_USE},
        {WSAEADDRNOTAVAIL,  Error::ADDR_NOT_AVAIL},
        {WSAENETUNREACH,    Error::NET_UNREACHABLE},
        {WSAEHOSTUNREACH,   Error::HOST_UNREACHABLE},
        {WSAEINVAL,         Error::INVALID_ARG}
    };
#else
    static inline const std::map<int, Error> errorMap = {
        {0,                 Error::NONE},
        {EAGAIN,            Error::WOULD_BLOCK},
        {EWOULDBLOCK,       Error::WOULD_BLOCK},
        {EINTR,             Error::INTERRUPTED},
        {ECONNREFUSED,      Error::CONN_REFUSED},
        {ECONNRESET,        Error::CONN_RESET},
        {ECONNABORTED,      Error::CONN_ABORTED},
        {ETIMEDOUT,         Error::TIMED_OUT},
        {EINPROGRESS,       Error::IN_PROGRESS},
        {EALREADY,          Error::ALREADY},
        {ENOTCONN,          Error::NOT_CONNECTED},
        {EADDRINUSE,        Error::ADDR_IN_USE},
        {EADDRNOTAVAIL,     Error::ADDR_NOT_AVAIL},
        {ENETUNREACH,       Error::NET_UNREACHABLE},
        {EHOSTUNREACH,      Error::HOST_UNREACHABLE},
        {EINVAL,            Error::INVALID_ARG}
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

    void bind(uint16_t port, const char* ip = nullptr);

    void listen(int backlog = DEFAULT_BACKLOG);

    Socket accept();

    void connect(const char* ip, uint16_t port);

    int send(const char* data, size_t len);

    int sendCommited(const char* data, size_t len, size_t maxRetryCount);

    int sendLoop(char* buffer, size_t len, size_t totalStart, size_t maxRetryCount,
        std::function<bool(char*&, size_t&, size_t, size_t&)> handler);

    //returns available data size
    u_long checkDataAvailable();

    //returns true if there is data in the queue, returns false when timeout expires
    template<typename Duration>
    bool waitForData(const Duration& timeout) {

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

        auto secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout - secs);
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
            if(getLastError() == Error::INTERRUPTED)
                return false;  // Interrupted by signal
            throw std::runtime_error("Select failed: " + getLastErrorString());
        }

        return result > 0;  // True if data available
    }

    int receive(char* buffer, size_t len);

    int receiveLoop(char* buffer, size_t len, size_t totalStart, size_t maxRetryCount,
        std::function<bool(char*&, size_t&, size_t, size_t&)> handler);

    void close();


    template<typename Duration>
    Socket& setTimeout(const Duration& timeout) {
        m_timeout = std::chrono::duration_cast<std::chrono::seconds>(timeout).count();
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

    Socket& setNonBlocking(bool nonBlocking = true);

    int getReceiveBufferSize();

    void setReceiveBufferSize(int size);

    static Error getLastError();

    static std::string getErrorString(Error error);

    static std::string getLastErrorString();
};

