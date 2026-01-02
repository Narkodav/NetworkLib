#pragma once
#include "Acceptor.h"
#include "IOContext.h"
#include "Session.h"

#include "JsonParser/Value.h"

namespace Network::HTTP
{
    class Server
    {
    public:
        using RequestHandlerFunction = std::function<std::unique_ptr<Response>(Request&)>;
		using ResponseHandlerFunction = std::function<std::unique_ptr<Message>(Response&)>;

        static inline const std::map<std::string, std::string> mimeTypes = {
            {".html", "text/html"},
            {".htm",  "text/html"},
            {".css",  "text/css"},
            {".js",   "application/javascript"},
            {".json", "application/json"},
            {".xml",  "application/xml"},
            {".txt",  "text/plain"},
            {".jpg",  "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".png",  "image/png"},
            {".gif",  "image/gif"},
            {".svg",  "image/svg+xml"},
            {".ico",  "image/x-icon"},
            {".pdf",  "application/pdf"},
            {".zip",  "application/zip"},
            {".gz",   "application/gzip"},
            {".woff", "font/woff"},
            {".woff2","font/woff2"},
            {".ttf",  "font/ttf"},
            {".eot",  "application/vnd.ms-fontobject"},
            {".mp3",  "audio/mpeg"},
            {".mp4",  "video/mp4"},
            {".doc",  "application/msword"},
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {".webm", "video/webm"},
            {".webp", "image/webp"},
            {".wasm", "application/wasm"},
        };

    private:
        IOContext& m_context;
        Acceptor m_acceptor;
        std::string m_name;

        std::atomic<uint64_t> m_temporaryFileCounter = 0;
        uint64_t m_sessionCounter = 0;

        //statistics
        size_t m_totalBytesSent = 0;
        size_t m_totalBytesReceived = 0;
        size_t m_totalRequests = 0;
        size_t m_activeSessions = 0;

        std::array<RequestHandlerFunction, static_cast<size_t>(Request::Method::Count)> m_handlers = {
            [this](Request& req) { return std::move(handleGet(req)); },
            [this](Request& req) { return std::move(handleConnect(req)); },
            [this](Request& req) { return std::move(handleDelete(req)); },
            [this](Request& req) { return std::move(handleHead(req)); },
            [this](Request& req) { return std::move(handleOptions(req)); },
            [this](Request& req) { return std::move(handlePatch(req)); },
            [this](Request& req) { return std::move(handlePost(req)); },
            [this](Request& req) { return std::move(handlePut(req)); },
            [this](Request& req) { return std::move(handleTrace(req)); },
            [this](Request& req) { return std::move(handleUnknown(req)); },
        };

        ResponseHandlerFunction m_responseHandler =
            [this](Response& res) { return std::move(handleResponse(res)); };

    public:

        Server(IOContext& context, int port, std::string_view name) :
            m_context(context), m_acceptor(context, port), m_name(name) {
        };

        void startBlocking();

        void accept();

        std::unique_ptr<Body> chooseBodyType(std::unique_ptr<Message>& msg);

        std::unique_ptr<Message> handleMessage(std::unique_ptr<Message>& msg) {

            if (msg->getType() == Message::Type::Request)
            {
                auto& req = static_cast<Request&>(*msg);
                return m_handlers[static_cast<size_t>(req.getMethod())](req);
            }
            else
            {
                return handleResponse(static_cast<Response&>(*msg));
            }
        }

        //default handlers
        std::unique_ptr<Response> handleGet(Request& req);
        std::unique_ptr<Response> handleConnect(Request& req);
        std::unique_ptr<Response> handleDelete(Request& req);
        std::unique_ptr<Response> handleHead(Request& req);
        std::unique_ptr<Response> handleOptions(Request& req);
        std::unique_ptr<Response> handlePatch(Request& req);
        std::unique_ptr<Response> handlePost(Request& req);
        std::unique_ptr<Response> handlePut(Request& req);
        std::unique_ptr<Response> handleTrace(Request& req);
        std::unique_ptr<Response> handleUnknown(Request& req);

        std::unique_ptr<Response> handleResponse(Response& req);

        std::string getMimeType(std::string_view path) {

            std::string ext = std::filesystem::path(path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            auto it = mimeTypes.find(ext);
            if (it != mimeTypes.end()) {
                return it->second;
            }

            return "application/octet-stream";
        }

        std::string_view getName() const {
            return m_name;
        };

        void setHandler(Request::Method method, RequestHandlerFunction handler) {
            m_handlers[static_cast<size_t>(method)] = handler;
        };

        void setResponseHandler(ResponseHandlerFunction handler) {
            m_responseHandler = handler;
        };
    };
}



