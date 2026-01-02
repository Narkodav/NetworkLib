#pragma once
#include "Common.h"

namespace Network {

    class Socket {
    public:
        struct AddressIn {
            uint16_t        sin_family;
            uint16_t        sin_port;
            uint32_t        sin_addr;
            uint8_t         sin_zero[8];
        };

#ifdef _WIN32
        typedef uint64_t Handle;  // 64-bit on 64-bit Windows
#else
        typedef int Handle;  // int on Unix/Linux
#endif

#ifdef _WIN32
        // Windows socket constants
        enum class Domain : int {
            Unspec = 0,    // AF_UNSPEC
            Unix = 1,    // AF_UNIX
            IPv4 = 2,    // AF_INET
            IPX = 6,    // AF_IPX
            AppleTalk = 16,   // AF_APPLETALK
            NetBIOS = 17,   // AF_NETBIOS
            IPv6 = 23,   // AF_INET6
            IrDA = 26,   // AF_IRDA
            Bluetooth = 32,    // AF_BTH

            Unknown = -1
        };

        enum class Type : int {
            Stream = 1,    // SOCK_STREAM
            Dgram = 2,    // SOCK_DGRAM
            Raw = 3,    // SOCK_RAW
            Rdm = 4,    // SOCK_RDM
            Seqpacket = 5,     // SOCK_SEQPACKET
            Unknown = -1
        };

        enum class Protocol : int {
            Default = 0,    // Let OS choose
            ICMP = 1,    // IPPROTO_ICMP
            TCP = 6,    // IPPROTO_TCP
            UDP = 17,   // IPPROTO_UDP
            IPv6 = 41,   // IPPROTO_IPV6
            IPv6_Routing = 43, // IPPROTO_ROUTING
            RawSocket = 255,   // IPPROTO_RAW
            Unknown = -1
        };

#else
        // Unix/Linux socket constants
        enum class Domain : int {
            Unspec = 0,    // AF_UNSPEC
            Unix = 1,    // AF_UNIX/AF_LOCAL
            IPv4 = 2,    // AF_INET
            IPv6 = 10,   // AF_INET6 on Linux
            Netlink = 16,   // AF_NETLINK
            Packet = 17,    // AF_PACKET
            Unknown = -1
        };

        enum class Type : int {
            Stream = 1,    // SOCK_STREAM
            Dgram = 2,    // SOCK_DGRAM
            Raw = 3,    // SOCK_RAW
            Rdm = 4,    // SOCK_RDM
            Seqpacket = 5,     // SOCK_SEQPACKET
            Unknown = -1
        };

        enum class Protocol : int {
            Default = 0,    // Let OS choose
            ICMP = 1,    // IPPROTO_ICMP
            TCP = 6,    // IPPROTO_TCP
            UDP = 17,   // IPPROTO_UDP
            RawSocket = 255,   // IPPROTO_RAW
            Unknown = -1
        };
#endif


#ifdef _WIN32
        // Windows-specific error codes (from winsock2.h)
        enum class Error {
            None = 0,      // NO_ERROR
            WouldBlock = 10035,  // WSAEWOULDBLOCK
            Interrupted = 10004,  // WSAEINTR
            Disconnected = 10057,  // WSAENOTCONN (closest to disconnected)
            ConnectionRefused = 10061,  // WSAECONNREFUSED
            ConnectionReset = 10054,  // WSAECONNRESET
            ConnectionAborted = 10053,  // WSAECONNABORTED
            TimedOut = 10060,  // WSAETIMEDOUT
            InProgress = 10036,  // WSAEINPROGRESS
            Already = 10037,  // WSAEALREADY
            NotConnected = 10057,  // WSAENOTCONN
            AddressInUse = 10048,  // WSAEADDRINUSE
            AddressNotAvailable = 10049,  // WSAEADDRNOTAVAIL
            NetworkUnreachable = 10051,  // WSAENETUNREACH
            HostUnreachable = 10065,  // WSAEHOSTUNREACH
            InvalidArgument = 10022,  // WSAEINVAL
            Unknown = -1      // Custom value for unknown errors
        };

#else
        enum class Error {
            None = 0,      // Success
            WouldBlock = 11,     // EAGAIN (same as EWOULDBLOCK on most systems)
            Interrupted = 4,      // EINTR
            Disconnected = 107,    // ENOTCONN (closest to disconnected)
            ConnectionRefused = 111,    // ECONNREFUSED
            ConnectionReset = 104,    // ECONNRESET
            ConnectionAborted = 103,    // ECONNABORTED
            TimedOut = 110,    // ETIMEDOUT
            InProgress = 115,    // EINPROGRESS
            Already = 114,    // EALREADY
            NotConnected = 107,    // ENOTCONN
            AddressInUse = 98,     // EADDRINUSE
            AddressNotAvailable = 99,     // EADDRNOTAVAIL
            NetworkUnreachable = 101,    // ENETUNREACH
            HostUnreachable = 113,    // EHOSTUNREACH
            InvalidArgument = 22,     // EINVAL
            Unknown = -1      // Custom value for unknown errors
        };
#endif

        static inline const std::map<Error, std::string> s_errorStrings = {
            {Error::None,                 "No error"},
            {Error::WouldBlock,           "Operation would block"},
            {Error::Interrupted,          "Operation interrupted"},
            {Error::Disconnected,         "Connection disconnected"},
            {Error::ConnectionRefused,    "Connection refused by peer"},
            {Error::ConnectionReset,      "Connection reset by peer"},
            {Error::ConnectionAborted,    "Connection aborted"},
            {Error::TimedOut,             "Operation timed out"},
            {Error::InProgress,           "Operation in progress"},
            {Error::Already,              "Operation already in progress"},
            {Error::NotConnected,         "Socket not connected"},
            {Error::AddressInUse,         "Address already in use"},
            {Error::AddressNotAvailable,  "Address not available"},
            {Error::NetworkUnreachable,   "Network is unreachable"},
            {Error::HostUnreachable,      "Host is unreachable"},
            {Error::InvalidArgument,      "Invalid argument"},
            {Error::Unknown,              "Unknown error"}
        };

#ifdef _WIN32
        static const std::map<int, Error> s_errorMap;
#else
        static const std::map<int, Error> s_errorMap;
#endif

        static constexpr int s_defaultBacklog = 4096;

    private:
        int m_timeout = 0;  // in seconds
        bool m_nonBlocking = false;

        Handle m_sockfd;
        Domain m_domain;
        Type m_type;
        Protocol m_protocol;
        AddressIn m_addr;

        bool m_isConnected = false;

    protected:
        // Constructor for accepted sockets
        Socket(Handle fd, AddressIn adr,
            int timeout = 30, bool nonBlocking = false) :
            m_sockfd(fd), m_addr(adr), m_isConnected(true),
            m_timeout(0), m_nonBlocking(false) {
        };

    public:
        Socket(Domain domain = Domain::IPv4, Type type = Type::Stream, Protocol protocol = Protocol::Default);

        ~Socket() {
            close();
        }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other) noexcept
        {
            m_sockfd = std::exchange(other.m_sockfd, -1);
            m_domain = std::exchange(other.m_domain, Domain::Unknown);
            m_type = std::exchange(other.m_type, Type::Unknown);
            m_protocol = std::exchange(other.m_protocol, Protocol::Unknown);
            m_addr = std::exchange(other.m_addr, {});
            m_isConnected = std::exchange(other.m_isConnected, false);
        }

        // Move assignment
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();  // Close existing socket if any
                m_sockfd = std::exchange(other.m_sockfd, -1);
                m_domain = std::exchange(other.m_domain, Domain::Unknown);
                m_type = std::exchange(other.m_type, Type::Unknown);
                m_protocol = std::exchange(other.m_protocol, Protocol::Unknown);
                m_addr = std::exchange(other.m_addr, {});
                m_isConnected = std::exchange(other.m_isConnected, false);
            }
            return *this;
        }

        void bind(uint16_t port, const char* ip = nullptr);

        void listen(int backlog = s_defaultBacklog);

        Socket accept();

        void connect(const char* ip, uint16_t port);

        int send(const char* data, size_t len);

        int sendCommited(const char* data, size_t len, size_t maxRetryCount);

        int sendLoop(char* buffer, size_t len, size_t totalStart, size_t maxRetryCount,
            std::function<bool(char*&, size_t&, size_t, size_t&)> handler);

        //returns available data size
        uint32_t checkDataAvailable();

        //returns true if there is data in the queue, returns false when timeout expires
        template<typename Duration>
        bool waitForData(const Duration& timeout) {
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
            auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout - secs);
            return waitForData(secs, microsecs);
        }

        bool waitForData(const std::chrono::seconds& secs, const std::chrono::microseconds& microsecs);

        int receive(char* buffer, size_t len);

        int receiveLoop(char* buffer, size_t len, size_t totalStart, size_t maxRetryCount,
            std::function<bool(char*&, size_t&, size_t, size_t&)> handler);

        void close();

        template<typename Duration>
        Socket& setTimeout(const Duration& timeout) {
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
            return setTimeout(secs);
        }

        Socket& setTimeout(const std::chrono::seconds& timeout);

        Socket& setNonBlocking(bool nonBlocking = true);

        int getReceiveBufferSize();

        void setReceiveBufferSize(int size);

        static Error getLastError();

        static std::string getErrorString(Error error);

        static std::string getLastErrorString();
    };
}