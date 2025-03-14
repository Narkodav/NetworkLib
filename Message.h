#pragma once
#include "Common.h"
#include "Socket.h"
#include "Body.h"

namespace http
{
    class Message {
    public:
        class Headers {
        public:
            enum class Standard {
                ACCEPT,
                ACCEPT_CHARSET,
                ACCEPT_ENCODING,
                ACCEPT_LANGUAGE,
                AUTHORIZATION,
                CACHE_CONTROL,
                CONNECTION,
                CONTENT_LENGTH,
                CONTENT_TYPE,
                COOKIE,
                DATE,
                HOST,
                IF_MATCH,
                IF_MODIFIED_SINCE,
                IF_NONE_MATCH,
                IF_RANGE,
                IF_UNMODIFIED_SINCE,
                LOCATION,
                MAX_FORWARDS,
                PRAGMA,
                PROXY_AUTHORIZATION,
                RANGE,
                REFERER,
                SERVER,
                TE,
                TRANSFER_ENCODING,
                UPGRADE,
                USER_AGENT,
                VIA,
                WARNING
            };

            static inline const std::map<Standard, std::string> headerToString = {
                {Standard::ACCEPT, "Accept"},
                {Standard::ACCEPT_CHARSET, "Accept-Charset"},
                {Standard::ACCEPT_ENCODING, "Accept-Encoding"},
                {Standard::ACCEPT_LANGUAGE, "Accept-Language"},
                {Standard::AUTHORIZATION, "Authorization"},
                {Standard::CACHE_CONTROL, "Cache-Control"},
                {Standard::CONNECTION, "Connection"},
                {Standard::CONTENT_LENGTH, "Content-Length"},
                {Standard::CONTENT_TYPE, "Content-Type"},
                {Standard::COOKIE, "Cookie"},
                {Standard::DATE, "Date"},
                {Standard::HOST, "Host"},
                {Standard::IF_MATCH, "If-Match"},
                {Standard::IF_MODIFIED_SINCE, "If-Modified-Since"},
                {Standard::IF_NONE_MATCH, "If-None-Match"},
                {Standard::IF_RANGE, "If-Range"},
                {Standard::IF_UNMODIFIED_SINCE, "If-Unmodified-Since"},
                {Standard::LOCATION, "Location"},
                {Standard::MAX_FORWARDS, "Max-Forwards"},
                {Standard::PRAGMA, "Pragma"},
                {Standard::PROXY_AUTHORIZATION, "Proxy-Authorization"},
                {Standard::RANGE, "Range"},
                {Standard::REFERER, "Referer"},
                {Standard::SERVER, "Server"},
                {Standard::TE, "TE"},
                {Standard::TRANSFER_ENCODING, "Transfer-Encoding"},
                {Standard::UPGRADE, "Upgrade"},
                {Standard::USER_AGENT, "User-Agent"},
                {Standard::VIA, "Via"},
                {Standard::WARNING, "Warning"}
            };

            static inline const std::map<std::string, Standard, CaseInsensetiveStringComparator> headerFromString = {
                {"accept", Standard::ACCEPT},
                {"accept-charset", Standard::ACCEPT_CHARSET},
                {"accept-encoding", Standard::ACCEPT_ENCODING},
                {"accept-language", Standard::ACCEPT_LANGUAGE},
                {"authorization", Standard::AUTHORIZATION},
                {"cache-control", Standard::CACHE_CONTROL},
                {"connection", Standard::CONNECTION},
                {"content-length", Standard::CONTENT_LENGTH},
                {"content-type", Standard::CONTENT_TYPE},
                {"cookie", Standard::COOKIE},
                {"date", Standard::DATE},
                {"host", Standard::HOST},
                {"if-match", Standard::IF_MATCH},
                {"if-modified-since", Standard::IF_MODIFIED_SINCE},
                {"if-none-match", Standard::IF_NONE_MATCH},
                {"if-range", Standard::IF_RANGE},
                {"if-unmodified-since", Standard::IF_UNMODIFIED_SINCE},
                {"location", Standard::LOCATION},
                {"max-forwards", Standard::MAX_FORWARDS},
                {"pragma", Standard::PRAGMA},
                {"proxy-authorization", Standard::PROXY_AUTHORIZATION},
                {"range", Standard::RANGE},
                {"referer", Standard::REFERER},
                {"server", Standard::SERVER},
                {"te", Standard::TE},
                {"transfer-encoding", Standard::TRANSFER_ENCODING},
                {"upgrade", Standard::UPGRADE},
                {"user-agent", Standard::USER_AGENT},
                {"via", Standard::VIA},
                {"warning", Standard::WARNING}
            };

            class iterator {
            private:
                std::map<Standard, std::string>::const_iterator standardIt;
                std::map<Standard, std::string>::const_iterator standardEnd;
                std::map<std::string, std::string, CaseInsensetiveStringComparator>::const_iterator customIt;
                std::map<std::string, std::string, CaseInsensetiveStringComparator>::const_iterator customEnd;
                bool isInStandard;

            public:
                iterator(
                    std::map<Standard, std::string>::const_iterator stdIt,
                    std::map<Standard, std::string>::const_iterator stdEnd,
                    std::map<std::string, std::string, CaseInsensetiveStringComparator>::const_iterator custIt,
                    std::map<std::string, std::string, CaseInsensetiveStringComparator>::const_iterator custEnd,
                    bool inStandard
                ) : standardIt(stdIt), standardEnd(stdEnd),
                    customIt(custIt), customEnd(custEnd),
                    isInStandard(inStandard) {
                }

                // Iterator operators
                iterator& operator++() {
                    if (isInStandard) {
                        ++standardIt;
                        if (standardIt == standardEnd) {
                            isInStandard = false;
                        }
                    }
                    else {
                        ++customIt;
                    }
                    return *this;
                }

                bool operator!=(const iterator& other) const {
                    if (isInStandard != other.isInStandard) return true;
                    if (isInStandard) {
                        return standardIt != other.standardIt;
                    }
                    return customIt != other.customIt;
                }

                // Return type for dereferencing
                std::pair<std::string, std::string> operator*() const {
                    if (isInStandard) {
                        return { headerToString.at(standardIt->first), standardIt->second };
                    }
                    return { customIt->first, customIt->second };
                }
            };

        private:

            std::map<Standard, std::string> m_standardHeaders;
            std::map<std::string, std::string, CaseInsensetiveStringComparator> m_customHeaders;

        public:

            void set(Standard header, const std::string& value) {
                m_standardHeaders[header] = value;
            }

            void set(const std::string& header, const std::string& value) {
                auto it = headerFromString.find(header);
                if (it != headerFromString.end())
                    m_standardHeaders[it->second] = value;
                else m_customHeaders[header] = value;
            }

            bool has(Standard header) const {
                return m_standardHeaders.contains(header);
            }

            bool has(const std::string& header) const {
                auto it = headerFromString.find(header);
                if (it != headerFromString.end())
                    return m_standardHeaders.contains(it->second);
                else return m_customHeaders.contains(header);
            }

            std::string get(Standard header) const {
                auto it = m_standardHeaders.find(header);
                return it != m_standardHeaders.end() ? it->second : "";
            }

            std::string get(const std::string& header) const {
                auto it = headerFromString.find(header);
                if (it != headerFromString.end())
                {
                    auto itFin = m_standardHeaders.find(it->second);
                    return itFin != m_standardHeaders.end() ? itFin->second : "";
                }
                else
                {
                    auto itFin = m_customHeaders.find(header);
                    return itFin != m_customHeaders.end() ? itFin->second : "";
                }
            }

            void remove(Standard header) {
                m_standardHeaders.erase(header);
            }

            void remove(const std::string& header) {
                auto it = headerFromString.find(header);
                if (it != headerFromString.end())
                    m_standardHeaders.erase(it->second);
                else
                    m_customHeaders.erase(header);
            }

            std::vector<std::string> getHeaderNames() const {
                std::vector<std::string> names;
                names.reserve(m_standardHeaders.size() + m_customHeaders.size());

                for (const auto& [header, _] : m_standardHeaders) {
                    names.push_back(headerToString.at(header));
                }
                for (const auto& [name, _] : m_customHeaders) {
                    names.push_back(name);
                }
                return names;
            }

            // Iteration support
            auto standardBegin() const { return m_standardHeaders.begin(); }
            auto standardEnd() const { return m_standardHeaders.end(); }
            auto customBegin() const { return m_customHeaders.begin(); }
            auto customEnd() const { return m_customHeaders.end(); }

            size_t size() const {
                return m_standardHeaders.size() + m_customHeaders.size();
            }

            bool empty() const {
                return m_standardHeaders.empty() && m_customHeaders.empty();
            }

            void clear() {
                m_standardHeaders.clear();
                m_customHeaders.clear();
            }

            // Utility methods
            std::string toString() const {
                std::stringstream ss;
                // Standard headers
                for (const auto& [header, value] : m_standardHeaders) {
                    ss << headerToString.at(header) << ": " << value << "\r\n";
                }
                // Custom headers
                for (const auto& [name, value] : m_customHeaders) {
                    ss << name << ": " << value << "\r\n";
                }
                return ss.str();
            }

            iterator begin() const {
                bool startInStandard = !m_standardHeaders.empty();
                return iterator(
                    m_standardHeaders.begin(), m_standardHeaders.end(),
                    m_customHeaders.begin(), m_customHeaders.end(),
                    startInStandard
                );
            }

            iterator end() const {
                return iterator(
                    m_standardHeaders.end(), m_standardHeaders.end(),
                    m_customHeaders.end(), m_customHeaders.end(),
                    false
                );
            }

        };

        enum class Type {
            UNKNOWN,
            REQUEST,
            RESPONSE
        };

        enum class TransferMethod {
            UNKNOWN,
            CONTENT_LENGTH,
            CHUNKED,
            CONNECTION_CLOSE,
            NONE,
        };

    protected:
        std::string version = "HTTP/1.1";
        Headers headers;
        std::unique_ptr<Body> body;

    public:
        virtual ~Message() = default;
        
        Headers& getHeaders() { return headers; };
        const Headers& getHeaders() const { return headers; };

        void setBody(std::unique_ptr<Body>&& content) { body = std::move(content); };

        std::unique_ptr<Body>& getBody() { return body; };
        const std::unique_ptr<Body>& getBody() const { return body; };
        void setVersion(const std::string& ver) { version = ver; };

        virtual Type getType() const { return Type::UNKNOWN; };

        virtual std::string getFirstLine() const = 0;
    };

    class Request : public Message {
    public:
        enum class Method
        {
            UNKNOWN,
            GET,        // Retrieve a resource
            HEAD,       // Same as GET but returns only headers
            POST,       // Submit data to be processed
            PUT,        // Upload a resource
            DELETE_,    // Remove a resource
            CONNECT,    // Establish a tunnel to the server
            OPTIONS,    // Describe communication options
            TRACE,      // Perform a message loop-back test
            PATCH       // Partially modify a resource
        };

        static inline const std::map<Method, std::string> methodToStringMap = {
            {Method::UNKNOWN, "UNKNOWN"},
            {Method::GET, "GET"},
            {Method::HEAD, "HEAD"},
            {Method::POST, "POST"},
            {Method::PUT, "PUT"},
            {Method::DELETE_, "DELETE"},
            {Method::CONNECT, "CONNECT"},
            {Method::OPTIONS, "OPTIONS"},
            {Method::TRACE, "TRACE"},
            {Method::PATCH, "PATCH"}
        };

        static inline const std::map<std::string, Method,
            CaseInsensetiveStringComparator> methodFromStringMap = {
                {"UNKNOWN", Method::UNKNOWN},
                {"GET", Method::GET},
                {"HEAD", Method::HEAD},
                {"POST", Method::POST},
                {"PUT", Method::PUT},
                {"DELETE", Method::DELETE_},
                {"CONNECT", Method::CONNECT},
                {"OPTIONS", Method::OPTIONS},
                {"TRACE", Method::TRACE},
                {"PATCH", Method::PATCH}
        };

    private:
        Method method;
        std::string uri;

    public:
        Request() = default;
        void setMethod(const Method& m) { method = m; }
        void setUri(const std::string& u) { uri = u; }
        const Method& getMethod() const { return method; }
        const std::string& getUri() const { return uri; }

        virtual Type getType() const override { return Type::REQUEST; };

        static Method stringToMethod(const std::string& s) {
            auto it = methodFromStringMap.find(s);
            return it != methodFromStringMap.end() ? it->second : Method::UNKNOWN;
        };

        static const std::string& methodToString(Method m) {
            auto it = methodToStringMap.find(m);
            return it != methodToStringMap.end() ? it->second : "UNKNOWN";
        };

        virtual std::string getFirstLine() const;
    };

    class Response : public Message {
    public:
        enum class StatusCode
        {
            UNKNOWN = 0,

            // 1xx Informational
            CONTINUE = 100,
            SWITCHING_PROTOCOLS = 101,
            PROCESSING = 102,
            EARLY_HINTS = 103,

            // 2xx Success
            OK = 200,
            CREATED = 201,
            ACCEPTED = 202,
            NON_AUTHORITATIVE_INFORMATION = 203,
            NO_CONTENT = 204,
            RESET_CONTENT = 205,
            PARTIAL_CONTENT = 206,

            // 3xx Redirection
            MULTIPLE_CHOICES = 300,
            MOVED_PERMANENTLY = 301,
            FOUND = 302,
            SEE_OTHER = 303,
            NOT_MODIFIED = 304,
            TEMPORARY_REDIRECT = 307,
            PERMANENT_REDIRECT = 308,

            // 4xx Client Error
            BAD_REQUEST = 400,
            UNAUTHORIZED = 401,
            PAYMENT_REQUIRED = 402,
            FORBIDDEN = 403,
            NOT_FOUND = 404,
            METHOD_NOT_ALLOWED = 405,
            NOT_ACCEPTABLE = 406,
            PROXY_AUTHENTICATION_REQUIRED = 407,
            REQUEST_TIMEOUT = 408,
            CONFLICT = 409,
            GONE = 410,
            LENGTH_REQUIRED = 411,
            PRECONDITION_FAILED = 412,
            PAYLOAD_TOO_LARGE = 413,
            URI_TOO_LONG = 414,
            UNSUPPORTED_MEDIA_TYPE = 415,
            RANGE_NOT_SATISFIABLE = 416,
            EXPECTATION_FAILED = 417,
            IM_A_TEAPOT = 418,
            UNPROCESSABLE_ENTITY = 422,
            TOO_EARLY = 425,
            UPGRADE_REQUIRED = 426,
            PRECONDITION_REQUIRED = 428,
            TOO_MANY_REQUESTS = 429,
            REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
            UNAVAILABLE_FOR_LEGAL_REASONS = 451,

            // 5xx Server Error
            INTERNAL_SERVER_ERROR = 500,
            NOT_IMPLEMENTED = 501,
            BAD_GATEWAY = 502,
            SERVICE_UNAVAILABLE = 503,
            GATEWAY_TIMEOUT = 504,
            HTTP_VERSION_NOT_SUPPORTED = 505,
            VARIANT_ALSO_NEGOTIATES = 506,
            INSUFFICIENT_STORAGE = 507,
            LOOP_DETECTED = 508,
            NOT_EXTENDED = 510,
            NETWORK_AUTHENTICATION_REQUIRED = 511
        };

        static inline const std::map<StatusCode, std::string> statusCodeToStringMap = {
            {StatusCode::UNKNOWN, "Unknown"},

            // 1xx Informational
            {StatusCode::CONTINUE, "Continue"},
            {StatusCode::SWITCHING_PROTOCOLS, "Switching Protocols"},
            {StatusCode::PROCESSING, "Processing"},
            {StatusCode::EARLY_HINTS, "Early Hints"},

            // 2xx Success
            {StatusCode::OK, "OK"},
            {StatusCode::CREATED, "Created"},
            {StatusCode::ACCEPTED, "Accepted"},
            {StatusCode::NON_AUTHORITATIVE_INFORMATION, "Non-Authoritative Information"},
            {StatusCode::NO_CONTENT, "No Content"},
            {StatusCode::RESET_CONTENT, "Reset Content"},
            {StatusCode::PARTIAL_CONTENT, "Partial Content"},

            // 3xx Redirection
            {StatusCode::MULTIPLE_CHOICES, "Multiple Choices"},
            {StatusCode::MOVED_PERMANENTLY, "Moved Permanently"},
            {StatusCode::FOUND, "Found"},
            {StatusCode::SEE_OTHER, "See Other"},
            {StatusCode::NOT_MODIFIED, "Not Modified"},
            {StatusCode::TEMPORARY_REDIRECT, "Temporary Redirect"},
            {StatusCode::PERMANENT_REDIRECT, "Permanent Redirect"},

            // 4xx Client Error
            {StatusCode::BAD_REQUEST, "Bad Request"},
            {StatusCode::UNAUTHORIZED, "Unauthorized"},
            {StatusCode::PAYMENT_REQUIRED, "Payment Required"},
            {StatusCode::FORBIDDEN, "Forbidden"},
            {StatusCode::NOT_FOUND, "Not Found"},
            {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
            {StatusCode::NOT_ACCEPTABLE, "Not Acceptable"},
            {StatusCode::PROXY_AUTHENTICATION_REQUIRED, "Proxy Authentication Required"},
            {StatusCode::REQUEST_TIMEOUT, "Request Timeout"},
            {StatusCode::CONFLICT, "Conflict"},
            {StatusCode::GONE, "Gone"},
            {StatusCode::LENGTH_REQUIRED, "Length Required"},
            {StatusCode::PRECONDITION_FAILED, "Precondition Failed"},
            {StatusCode::PAYLOAD_TOO_LARGE, "Payload Too Large"},
            {StatusCode::URI_TOO_LONG, "URI Too Long"},
            {StatusCode::UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type"},
            {StatusCode::RANGE_NOT_SATISFIABLE, "Range Not Satisfiable"},
            {StatusCode::EXPECTATION_FAILED, "Expectation Failed"},
            {StatusCode::IM_A_TEAPOT, "I'm a teapot"},
            {StatusCode::UNPROCESSABLE_ENTITY, "Unprocessable Entity"},
            {StatusCode::TOO_EARLY, "Too Early"},
            {StatusCode::UPGRADE_REQUIRED, "Upgrade Required"},
            {StatusCode::PRECONDITION_REQUIRED, "Precondition Required"},
            {StatusCode::TOO_MANY_REQUESTS, "Too Many Requests"},
            {StatusCode::REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large"},
            {StatusCode::UNAVAILABLE_FOR_LEGAL_REASONS, "Unavailable For Legal Reasons"},

            // 5xx Server Error
            {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
            {StatusCode::NOT_IMPLEMENTED, "Not Implemented"},
            {StatusCode::BAD_GATEWAY, "Bad Gateway"},
            {StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"},
            {StatusCode::GATEWAY_TIMEOUT, "Gateway Timeout"},
            {StatusCode::HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"},
            {StatusCode::VARIANT_ALSO_NEGOTIATES, "Variant Also Negotiates"},
            {StatusCode::INSUFFICIENT_STORAGE, "Insufficient Storage"},
            {StatusCode::LOOP_DETECTED, "Loop Detected"},
            {StatusCode::NOT_EXTENDED, "Not Extended"},
            {StatusCode::NETWORK_AUTHENTICATION_REQUIRED, "Network Authentication Required"}
        };

        static inline const std::map<std::string, StatusCode> statusCodeFromStringMap = {
            {"0", StatusCode::UNKNOWN},

            // 1xx Informational
            {"100", StatusCode::CONTINUE},
            {"101", StatusCode::SWITCHING_PROTOCOLS},
            {"102", StatusCode::PROCESSING},
            {"103", StatusCode::EARLY_HINTS},

            // 2xx Success
            {"200", StatusCode::OK},
            {"201", StatusCode::CREATED},
            {"202", StatusCode::ACCEPTED},
            {"203", StatusCode::NON_AUTHORITATIVE_INFORMATION},
            {"204", StatusCode::NO_CONTENT},
            {"205", StatusCode::RESET_CONTENT},
            {"206", StatusCode::PARTIAL_CONTENT},

            // 3xx Redirection
            {"300", StatusCode::MULTIPLE_CHOICES},
            {"301", StatusCode::MOVED_PERMANENTLY},
            {"302", StatusCode::FOUND},
            {"303", StatusCode::SEE_OTHER},
            {"304", StatusCode::NOT_MODIFIED},
            {"307", StatusCode::TEMPORARY_REDIRECT},
            {"308", StatusCode::PERMANENT_REDIRECT},

            // 4xx Client Error
            {"400", StatusCode::BAD_REQUEST},
            {"401", StatusCode::UNAUTHORIZED},
            {"402", StatusCode::PAYMENT_REQUIRED},
            {"403", StatusCode::FORBIDDEN},
            {"404", StatusCode::NOT_FOUND},
            {"405", StatusCode::METHOD_NOT_ALLOWED},
            {"406", StatusCode::NOT_ACCEPTABLE},
            {"407", StatusCode::PROXY_AUTHENTICATION_REQUIRED},
            {"408", StatusCode::REQUEST_TIMEOUT},
            {"409", StatusCode::CONFLICT},
            {"410", StatusCode::GONE},
            {"411", StatusCode::LENGTH_REQUIRED},
            {"412", StatusCode::PRECONDITION_FAILED},
            {"413", StatusCode::PAYLOAD_TOO_LARGE},
            {"414", StatusCode::URI_TOO_LONG},
            {"415", StatusCode::UNSUPPORTED_MEDIA_TYPE},
            {"416", StatusCode::RANGE_NOT_SATISFIABLE},
            {"417", StatusCode::EXPECTATION_FAILED},
            {"418", StatusCode::IM_A_TEAPOT},
            {"422", StatusCode::UNPROCESSABLE_ENTITY},
            {"425", StatusCode::TOO_EARLY},
            {"426", StatusCode::UPGRADE_REQUIRED},
            {"428", StatusCode::PRECONDITION_REQUIRED},
            {"429", StatusCode::TOO_MANY_REQUESTS},
            {"431", StatusCode::REQUEST_HEADER_FIELDS_TOO_LARGE},
            {"451", StatusCode::UNAVAILABLE_FOR_LEGAL_REASONS},

            // 5xx Server Error
            {"500", StatusCode::INTERNAL_SERVER_ERROR},
            {"501", StatusCode::NOT_IMPLEMENTED},
            {"502", StatusCode::BAD_GATEWAY},
            {"503", StatusCode::SERVICE_UNAVAILABLE},
            {"504", StatusCode::GATEWAY_TIMEOUT},
            {"505", StatusCode::HTTP_VERSION_NOT_SUPPORTED},
            {"506", StatusCode::VARIANT_ALSO_NEGOTIATES},
            {"507", StatusCode::INSUFFICIENT_STORAGE},
            {"508", StatusCode::LOOP_DETECTED},
            {"510", StatusCode::NOT_EXTENDED},
            {"511", StatusCode::NETWORK_AUTHENTICATION_REQUIRED}
        };

    private:
        StatusCode statusCode;
        std::string statusMessage;

    public:
        Response() = default;
        void setStatusCode(StatusCode code) { statusCode = code; }
        void setStatusMessage(const std::string& msg) { statusMessage = msg; }
        StatusCode getStatusCode() const { return statusCode; }
        const std::string& getStatusMessage() const { return statusMessage; }

        virtual Type getType() const override { return Type::RESPONSE; };

        static StatusCode stringToStatusCode(const std::string& s) {
            auto it = statusCodeFromStringMap.find(s);
            return it != statusCodeFromStringMap.end() ? it->second : StatusCode::UNKNOWN;
        };

        static const std::string& statusCodeToString(StatusCode m) {
            auto it = statusCodeToStringMap.find(m);
            return it != statusCodeToStringMap.end() ? it->second : "UNKNOWN";
        };

        virtual std::string getFirstLine() const;
    };

    using MessagePtr = std::unique_ptr<Message>; //a unique pointer to a message

}