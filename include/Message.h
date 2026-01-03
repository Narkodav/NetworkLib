#pragma once
#include "Common.h"
#include "Socket.h"
#include "Body.h"

namespace Network::HTTP
{
    class Message {
    public:
        class Headers {
        public:
            enum class Standard {
                Accept = 0,
                AcceptCharset,
                AcceptEncoding,
                AcceptLanguage,
                Authorization,
                CacheControl,
                Connection,
                ContentLength,
                ContentType,
                Cookie,
                Date,
                Host,
                IfMatch,
                IfModifiedSince,
                IfNoneMatch,
                IfRange,
                IfUnmodifiedSince,
                Location,
                MaxForwards,
                Pragma,
                ProxyAuthorization,
                Range,
                Referer,
                Server,
                Te,
                TransferEncoding,
                Upgrade,
                UserAgent,
                Via,
                Warning,
				AccessControlAllowOrigin,
				AccessControlAllowMethods,
				AccessControlAllowHeaders,
                Count
            };

            static inline const std::array<std::string, static_cast<size_t>(Standard::Count)> s_headerToString = {
                "Accept",
				"Accept-Charset",
				"Accept-Encoding",
				"Accept-Language",
				"Authorization",
				"Cache-Control",
				"Connection",
				"Content-Length",
				"Content-Type",
				"Cookie",
				"Date",
				"Host",
				"If-Match",
				"If-Modified-Since",
				"If-None-Match",
				"If-Range",
				"If-Unmodified-Since",
				"Location",
				"Max-Forwards",
				"Pragma",
				"Proxy-Authorization",
				"Range",
				"Referer",
				"Server",
				"TE",
				"Transfer-Encoding",
				"Upgrade",
				"User-Agent",
				"Via",
				"Warning",
                "Access-Control-Allow-Origin",
				"Access-Control-Allow-Methods",
				"Access-Control-Allow-Headers"
            };

            static inline const std::unordered_map<std::string, Standard, 
                Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator> s_headerFromString = {
                {"accept", Standard::Accept},
                {"accept-charset", Standard::AcceptCharset},
                {"accept-encoding", Standard::AcceptEncoding},
                {"accept-language", Standard::AcceptLanguage},
                {"authorization", Standard::Authorization},
                {"cache-control", Standard::CacheControl},
                {"connection", Standard::Connection},
                {"content-length", Standard::ContentLength},
                {"content-type", Standard::ContentType},
                {"cookie", Standard::Cookie},
                {"date", Standard::Date},
                {"host", Standard::Host},
                {"if-match", Standard::IfMatch},
                {"if-modified-since", Standard::IfModifiedSince},
                {"if-none-match", Standard::IfNoneMatch},
                {"if-range", Standard::IfRange},
                {"if-unmodified-since", Standard::IfUnmodifiedSince},
                {"location", Standard::Location},
                {"max-forwards", Standard::MaxForwards},
                {"pragma", Standard::Pragma},
                {"proxy-authorization", Standard::ProxyAuthorization},
                {"range", Standard::Range},
                {"referer", Standard::Referer},
                {"server", Standard::Server},
                {"te", Standard::Te},
                {"transfer-encoding", Standard::TransferEncoding},
                {"upgrade", Standard::Upgrade},
                {"user-agent", Standard::UserAgent},
                {"via", Standard::Via},
                {"warning", Standard::Warning},
                {"Access-Control-Allow-Origin", Standard::AccessControlAllowOrigin},
				{"Access-Control-Allow-Methods", Standard::AccessControlAllowMethods},
				{"Access-Control-Allow-Headers", Standard::AccessControlAllowHeaders}
            };

            class iterator {
            private:
                std::unordered_map<Standard, std::string>::const_iterator standardIt;
                std::unordered_map<Standard, std::string>::const_iterator standardEnd;
                std::unordered_map<std::string, std::string,
                    Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator>::const_iterator customIt;
                std::unordered_map<std::string, std::string,
                    Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator>::const_iterator customEnd;
                bool isInStandard;

            public:
                iterator(
                    std::unordered_map<Standard, std::string>::const_iterator stdIt,
                    std::unordered_map<Standard, std::string>::const_iterator stdEnd,
                    std::unordered_map<std::string, std::string,
                    Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator>::const_iterator custIt,
                    std::unordered_map<std::string, std::string,
                    Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator>::const_iterator custEnd,
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
                        return { s_headerToString[static_cast<size_t>(standardIt->first)], standardIt->second };
                    }
                    return { customIt->first, customIt->second };
                }
            };

        private:

            std::unordered_map<Standard, std::string> m_standardHeaders;
            std::unordered_map<std::string, std::string,
                Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator> m_customHeaders;

        public:

            void set(Standard header, std::string_view value) {
                m_standardHeaders[header] = value;
            }

            void set(const std::string& header, std::string_view value) {
                auto it = s_headerFromString.find(header);
                if (it != s_headerFromString.end())
                    m_standardHeaders[it->second] = value;
                else m_customHeaders[header] = value;
            }

            bool has(Standard header) const {
                return m_standardHeaders.contains(header);
            }

            bool has(const std::string& header) const {
                auto it = s_headerFromString.find(header);
                if (it != s_headerFromString.end())
                    return m_standardHeaders.contains(it->second);
                else return m_customHeaders.contains(header);
            }

            std::string get(Standard header) const {
                auto it = m_standardHeaders.find(header);
                return it != m_standardHeaders.end() ? it->second : "";
            }

            std::string get(const std::string& header) const {
                auto it = s_headerFromString.find(header);
                if (it != s_headerFromString.end())
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
                auto it = s_headerFromString.find(header);
                if (it != s_headerFromString.end())
                    m_standardHeaders.erase(it->second);
                else
                    m_customHeaders.erase(header);
            }

            std::vector<std::string> getHeaderNames() const {
                std::vector<std::string> names;
                names.reserve(m_standardHeaders.size() + m_customHeaders.size());

                for (const auto& [header, _] : m_standardHeaders) {
                    names.push_back(s_headerToString[static_cast<size_t>(header)]);
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
                    ss << s_headerToString[static_cast<size_t>(header)] << ": " << value << "\r\n";
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

            static std::string standardToString(Standard header) {
                return s_headerToString[static_cast<size_t>(header)];
            }

            static Standard stringToStandard(const std::string& header) {
                auto it = s_headerFromString.find(header);
                return it != s_headerFromString.end() ? it->second : Standard::Count;
			}
        };

        enum class Type {
            Unknown,
            Request,
            Response
        };

        enum class TransferMethod {
            Unknown,
            ContentLength,
            Chunked,
            ConnectionClose,
            None,
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

        virtual Type getType() const { return Type::Unknown; };

        virtual std::string getFirstLine() const = 0;
    };

    class Request : public Message {
    public:
        enum class Method
        {
            Unknown = 0,
            Get,        // Retrieve a resource
            Head,       // Same as GET but returns only headers
            Post,       // Submit data to be processed
            Put,        // Upload a resource
            Delete,     // Remove a resource
            Connect,    // Establish a tunnel to the server
            Options,    // Describe communication options
            Trace,      // Perform a message loop-back test
            Patch,       // Partially modify a resource
            Count
        };

        static inline const std::array<std::string, static_cast<size_t>(Method::Count)> s_methodToStringMap = {
            "UNKNOWN", "GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH"
        };

        static inline const std::unordered_map<std::string, Method,
            Detail::CaseInsensitiveHasher, Detail::CaseInsensitiveStringComparator> s_methodFromStringMap = {
                {"UNKNOWN", Method::Unknown},
                {"GET", Method::Get},
                {"HEAD", Method::Head},
                {"POST", Method::Post},
                {"PUT", Method::Put},
                {"DELETE", Method::Delete},
                {"CONNECT", Method::Connect},
                {"OPTIONS", Method::Options},
                {"TRACE", Method::Trace},
                {"PATCH", Method::Patch}
        };

    private:
        Method method;
        std::string uri;

    public:
        Request() = default;
        void setMethod(const Method& m) { method = m; }
        void setUri(const std::string& u) { uri = u; }
        const Method& getMethod() const { return method; }
        std::string_view getUri() const { return uri; }

        virtual Type getType() const override { return Type::Request; };

        static Method stringToMethod(std::string_view s) {
            auto it = s_methodFromStringMap.find(s);
            return it != s_methodFromStringMap.end() ? it->second : Method::Unknown;
        };

        static std::string_view methodToString(Method m) {
            if (m < Method::Count)
                return s_methodToStringMap[static_cast<size_t>(m)].c_str();
            return s_methodToStringMap[static_cast<size_t>(Method::Unknown)];
        };

        virtual std::string getFirstLine() const;
    };

    class Response : public Message {
    public:
        enum class StatusCode
        {
            Unknown = 0,

            // 1xx Informational
            Continue = 100,
            SwitchingProtocols = 101,
            Processing = 102,
            EarlyHints = 103,

            // 2xx Success
            Ok = 200,
            Created = 201,
            Accepted = 202,
            NonAuthoritativeInformation = 203,
            NoContent = 204,
            ResetContent = 205,
            PartialContent = 206,

            // 3xx Redirection
            MultipleChoices = 300,
            MovedPermanently = 301,
            Found = 302,
            SeeOther = 303,
            NotModified = 304,
            TemporaryRedirect = 307,
            PermanentRedirect = 308,

            // 4xx Client Error
            BadRequest = 400,
            Unauthorized = 401,
            PaymentRequired = 402,
            Forbidden = 403,
            NotFound = 404,
            MethodNotAllowed = 405,
            NotAcceptable = 406,
            ProxyAuthenticationRequired = 407,
            RequestTimeout = 408,
            Conflict = 409,
            Gone = 410,
            LengthRequired = 411,
            PreconditionFailed = 412,
            PayloadTooLarge = 413,
            UriTooLong = 414,
            UnsupportedMediaType = 415,
            RangeNotSatisfiable = 416,
            ExpectationFailed = 417,
            ImATeapot = 418,
            UnprocessableEntity = 422,
            TooEarly = 425,
            UpgradeRequired = 426,
            PreconditionRequired = 428,
            TooManyRequests = 429,
            RequestHeaderFieldsTooLarge = 431,
            UnavailableForLegalReasons = 451,

            // 5xx Server Error
            InternalServerError = 500,
            NotImplemented = 501,
            BadGateway = 502,
            ServiceUnavailable = 503,
            GatewayTimeout = 504,
            HttpVersionNotSupported = 505,
            VariantAlsoNegotiates = 506,
            InsufficientStorage = 507,
            LoopDetected = 508,
            NotExtended = 510,
            NetworkAuthenticationRequired = 511
        };

        static inline const std::unordered_map<StatusCode, std::string> s_statusCodeToStringMap = {
            {StatusCode::Unknown, "Unknown"},

            // 1xx Informational
            {StatusCode::Continue, "Continue"},
            {StatusCode::SwitchingProtocols, "Switching Protocols"},
            {StatusCode::Processing, "Processing"},
            {StatusCode::EarlyHints, "Early Hints"},

            // 2xx Success
            {StatusCode::Ok, "OK"},
            {StatusCode::Created, "Created"},
            {StatusCode::Accepted, "Accepted"},
            {StatusCode::NonAuthoritativeInformation, "Non-Authoritative Information"},
            {StatusCode::NoContent, "No Content"},
            {StatusCode::ResetContent, "Reset Content"},
            {StatusCode::PartialContent, "Partial Content"},

            // 3xx Redirection
            {StatusCode::MultipleChoices, "Multiple Choices"},
            {StatusCode::MovedPermanently, "Moved Permanently"},
            {StatusCode::Found, "Found"},
            {StatusCode::SeeOther, "See Other"},
            {StatusCode::NotModified, "Not Modified"},
            {StatusCode::TemporaryRedirect, "Temporary Redirect"},
            {StatusCode::PermanentRedirect, "Permanent Redirect"},

            // 4xx Client Error
            {StatusCode::BadRequest, "Bad Request"},
            {StatusCode::Unauthorized, "Unauthorized"},
            {StatusCode::PaymentRequired, "Payment Required"},
            {StatusCode::Forbidden, "Forbidden"},
            {StatusCode::NotFound, "Not Found"},
            {StatusCode::MethodNotAllowed, "Method Not Allowed"},
            {StatusCode::NotAcceptable, "Not Acceptable"},
            {StatusCode::ProxyAuthenticationRequired, "Proxy Authentication Required"},
            {StatusCode::RequestTimeout, "Request Timeout"},
            {StatusCode::Conflict, "Conflict"},
            {StatusCode::Gone, "Gone"},
            {StatusCode::LengthRequired, "Length Required"},
            {StatusCode::PreconditionFailed, "Precondition Failed"},
            {StatusCode::PayloadTooLarge, "Payload Too Large"},
            {StatusCode::UriTooLong, "URI Too Long"},
            {StatusCode::UnsupportedMediaType, "Unsupported Media Type"},
            {StatusCode::RangeNotSatisfiable, "Range Not Satisfiable"},
            {StatusCode::ExpectationFailed, "Expectation Failed"},
            {StatusCode::ImATeapot, "I'm a teapot"},
            {StatusCode::UnprocessableEntity, "Unprocessable Entity"},
            {StatusCode::TooEarly, "Too Early"},
            {StatusCode::UpgradeRequired, "Upgrade Required"},
            {StatusCode::PreconditionRequired, "Precondition Required"},
            {StatusCode::TooManyRequests, "Too Many Requests"},
            {StatusCode::RequestHeaderFieldsTooLarge, "Request Header Fields Too Large"},
            {StatusCode::UnavailableForLegalReasons, "Unavailable For Legal Reasons"},

            // 5xx Server Error
            {StatusCode::InternalServerError, "Internal Server Error"},
            {StatusCode::NotImplemented, "Not Implemented"},
            {StatusCode::BadGateway, "Bad Gateway"},
            {StatusCode::ServiceUnavailable, "Service Unavailable"},
            {StatusCode::GatewayTimeout, "Gateway Timeout"},
            {StatusCode::HttpVersionNotSupported, "HTTP Version Not Supported"},
            {StatusCode::VariantAlsoNegotiates, "Variant Also Negotiates"},
            {StatusCode::InsufficientStorage, "Insufficient Storage"},
            {StatusCode::LoopDetected, "Loop Detected"},
            {StatusCode::NotExtended, "Not Extended"},
            {StatusCode::NetworkAuthenticationRequired, "Network Authentication Required"}
        };

        static inline const std::unordered_map<std::string, StatusCode> s_statusCodeFromStringMap = {
            {"0", StatusCode::Unknown},

            // 1xx Informational
            {"100", StatusCode::Continue},
            {"101", StatusCode::SwitchingProtocols},
            {"102", StatusCode::Processing},
            {"103", StatusCode::EarlyHints},

            // 2xx Success
            {"200", StatusCode::Ok},
            {"201", StatusCode::Created},
            {"202", StatusCode::Accepted},
            {"203", StatusCode::NonAuthoritativeInformation},
            {"204", StatusCode::NoContent},
            {"205", StatusCode::ResetContent},
            {"206", StatusCode::PartialContent},

            // 3xx Redirection
            {"300", StatusCode::MultipleChoices},
            {"301", StatusCode::MovedPermanently},
            {"302", StatusCode::Found},
            {"303", StatusCode::SeeOther},
            {"304", StatusCode::NotModified},
            {"307", StatusCode::TemporaryRedirect},
            {"308", StatusCode::PermanentRedirect},

            // 4xx Client Error
            {"400", StatusCode::BadRequest},
            {"401", StatusCode::Unauthorized},
            {"402", StatusCode::PaymentRequired},
            {"403", StatusCode::Forbidden},
            {"404", StatusCode::NotFound},
            {"405", StatusCode::MethodNotAllowed},
            {"406", StatusCode::NotAcceptable},
            {"407", StatusCode::ProxyAuthenticationRequired},
            {"408", StatusCode::RequestTimeout},
            {"409", StatusCode::Conflict},
            {"410", StatusCode::Gone},
            {"411", StatusCode::LengthRequired},
            {"412", StatusCode::PreconditionFailed},
            {"413", StatusCode::PayloadTooLarge},
            {"414", StatusCode::UriTooLong},
            {"415", StatusCode::UnsupportedMediaType},
            {"416", StatusCode::RangeNotSatisfiable},
            {"417", StatusCode::ExpectationFailed},
            {"418", StatusCode::ImATeapot},
            {"422", StatusCode::UnprocessableEntity},
            {"425", StatusCode::TooEarly},
            {"426", StatusCode::UpgradeRequired},
            {"428", StatusCode::PreconditionRequired},
            {"429", StatusCode::TooManyRequests},
            {"431", StatusCode::RequestHeaderFieldsTooLarge},
            {"451", StatusCode::UnavailableForLegalReasons},

            // 5xx Server Error
            {"500", StatusCode::InternalServerError},
            {"501", StatusCode::NotImplemented},
            {"502", StatusCode::BadGateway},
            {"503", StatusCode::ServiceUnavailable},
            {"504", StatusCode::GatewayTimeout},
            {"505", StatusCode::HttpVersionNotSupported},
            {"506", StatusCode::VariantAlsoNegotiates},
            {"507", StatusCode::InsufficientStorage},
            {"508", StatusCode::LoopDetected},
            {"510", StatusCode::NotExtended},
            {"511", StatusCode::NetworkAuthenticationRequired}
        };

    private:
        StatusCode statusCode;
        std::string statusMessage;

    public:
        Response() = default;
        void setStatusCode(StatusCode code) { statusCode = code; }
        void setStatusMessage(std::string_view msg) { statusMessage = msg; }
        StatusCode getStatusCode() const { return statusCode; }
        const std::string& getStatusMessage() const { return statusMessage; }

        virtual Type getType() const override { return Type::Response; };

        static StatusCode stringToStatusCode(const std::string& s) {
            auto it = s_statusCodeFromStringMap.find(s);
            return it != s_statusCodeFromStringMap.end() ? it->second : StatusCode::Unknown;
        };

        static const std::string& statusCodeToString(StatusCode m) {
            auto it = s_statusCodeToStringMap.find(m);
            return it != s_statusCodeToStringMap.end() ? it->second : "UNKNOWN";
        };

        virtual std::string getFirstLine() const;
    };
}