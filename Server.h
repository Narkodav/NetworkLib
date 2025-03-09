#pragma once
#include "Acceptor.h"
#include "IOContext.h"
#include "Session.h"

namespace http
{
	class Server
	{
    public:
        using HandlerFunction = std::function<std::unique_ptr<Response>(Request&)>;

        // handlers could be loaded from a config, might implement later
        const std::map<Request::Method, HandlerFunction> handlers = {
            {Request::Method::GET,      [this](Request& req) { return std::move(handleGet(req)); }},
            {Request::Method::CONNECT,  [this](Request& req) { return std::move(handleConnect(req)); }},
            {Request::Method::DELETE_,  [this](Request& req) { return std::move(handleDelete(req)); }},
            {Request::Method::HEAD,     [this](Request& req) { return std::move(handleHead(req)); }},
            {Request::Method::OPTIONS,  [this](Request& req) { return std::move(handleOptions(req)); }},
            {Request::Method::PATCH,    [this](Request& req) { return std::move(handlePatch(req)); }},
            {Request::Method::POST,     [this](Request& req) { return std::move(handlePost(req)); }},
            {Request::Method::PUT,      [this](Request& req) { return std::move(handlePut(req)); }},
            {Request::Method::TRACE,    [this](Request& req) { return std::move(handleTrace(req)); }},
            {Request::Method::UNKNOWN,  [this](Request& req) { return std::move(handleUnknown(req)); }}
        };

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

	public:
		
		Server(IOContext& context, int port) :
			m_context(context), m_acceptor(context, port) {};

		void startBlocking();

		void accept();

        std::unique_ptr<Body> chooseBodyType(MessagePtr& msg);

        MessagePtr handleMessage(MessagePtr& msg) {

            if (msg->getType() == Message::Type::REQUEST)
            {
                auto& req = static_cast<Request&>(*msg);
                return handlers.find(req.getMethod())->second(req);
            }
            else
            {
                auto errorResponse = std::make_unique<Response>();
                errorResponse->setStatusCode(Response::StatusCode::BAD_REQUEST);
                errorResponse->setVersion("HTTP/1.1");

                auto& headers = errorResponse->getHeaders();
                headers.set(Message::Headers::Standard::CONTENT_TYPE, "text/plain");
                headers.set(Message::Headers::Standard::SERVER, m_name);

                auto body = std::make_unique<StringBody>();
                std::string text = "Bad Request: Server expects requests, not responses\n";
                headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(text.size()));
                body->write(text.data(), text.size());
                errorResponse->setBody(std::move(body));

                return errorResponse;
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

        std::string getMimeType(const std::string& path) {

            std::string ext = std::filesystem::path(path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            auto it = mimeTypes.find(ext);
            if (it != mimeTypes.end()) {
                return it->second;
            }

            return "application/octet-stream";
        }

	};
}



