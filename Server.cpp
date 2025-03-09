#include "Server.h"

namespace http
{
    void Server::startBlocking()
    {
        try
        {
            accept();
            m_context.run();
        }
        catch (std::exception e)
        {
            throw std::runtime_error("Server failed to start");
        }
    }

    void Server::accept()
    {
        m_acceptor.asyncAccept([this](Socket&& socket) {
            accept();
            m_activeSessions++;
            std::make_shared<Session>(
                std::move(socket),
                [this](MessagePtr& message) ->std::unique_ptr<Body> {
                    return chooseBodyType(message); //not thread safe
                },
                [this](std::unique_ptr<Message>& message) {
                    return handleMessage(message);
                }, std::to_string(m_sessionCounter)
                    )->startAssync(m_context, [this](const IOContext::SessionData& data) {
                    // Log session statistics
                    std::cout << "Session ended - Stats:\n"
                        << "  Bytes sent: " << data.bytesSent << "\n"
                        << "  Bytes received: " << data.bytesReceived << "\n"
                        << "  Requests handled: " << data.iterationCount << "\n";

                    // Update server metrics
                    m_totalBytesSent += data.bytesSent;
                    m_totalBytesReceived += data.bytesReceived;
                    m_totalRequests += data.iterationCount;
                    m_activeSessions--;
                        });

                m_sessionCounter++;
            });
    }

    std::unique_ptr<Body> Server::chooseBodyType(MessagePtr& msg) {
        std::unique_ptr<Body> body;
        auto& headers = msg->getHeaders();
        auto transferEncoding = headers.get(Message::Headers::Standard::TRANSFER_ENCODING);
        auto contentType = headers.get(Message::Headers::Standard::CONTENT_TYPE);
        if (transferEncoding == "chunked") {
            body = std::make_unique<FileBody>("Receives/Temporary" +
                std::to_string(m_temporaryFileCounter.load()) + ".bin"); //since we don't know the size default to file body
        }
        else if (contentType != "")
        {
            if (contentType == "application/json") {
                //body = std::make_unique<JsonBody>(); //need to implement body type
            }
            else if (contentType == "text/plain") {
                body = std::make_unique<StringBody>();
            }
            else if (contentType == "application/x-www-form-urlencoded") {
                //body = std::make_unique<FormBody>(); //need to implement body type
            }
            else if (contentType.find("multipart/form-data") != std::string::npos) {
                //body = std::make_unique<MultipartBody>(); //might be more complex
            }
            else if (contentType == "application/octet-stream") {
                //body = std::make_unique<BinaryBody>(); //need to implement body type
            }
            else {
                // Default to string body for unknown types
                body = std::make_unique<StringBody>();
            }
        }
        else {
            auto contentLength = headers.get(Message::Headers::Standard::CONTENT_LENGTH);
            if (contentLength != "") {
                size_t length = std::stoul(contentLength);
                if (length > 1024 * 1024) //1MB
                    body = std::make_unique<FileBody>("Receives/Temporary" +
                        std::to_string(m_temporaryFileCounter.load()) + ".bin");
            }
            else body = std::make_unique<StringBody>();
        }
        return std::move(body);
    }

    std::unique_ptr<Response> Server::handleGet(Request& req)
    {
        std::string target = std::string(req.getUri());
        if (target == "/") {
            target = "/index.html";
        }

        // Remove any .. to prevent directory traversal attacks
        if (target.find("..") != std::string::npos) {
            auto res = std::make_unique<Response>();
            res->setStatusCode(Response::StatusCode::FORBIDDEN);
            res->setVersion("HTTP/1.1");

            auto& headers = res->getHeaders();
            headers.set(Message::Headers::Standard::CONTENT_TYPE, "text/plain");
            headers.set(Message::Headers::Standard::SERVER, m_name);

            auto body = std::make_unique<StringBody>();
            std::string text = "Forbidden: Directory traversal attempt detected\n";
            headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(text.size()));
            body->write(text.data(), text.size());
            res->setBody(std::move(body));
            return res;
        }

        std::string filepath = "public" + target;

        try {
            if (!std::filesystem::exists(filepath)) {
                auto res = std::make_unique<Response>();
                res->setStatusCode(Response::StatusCode::NOT_FOUND);
                res->setVersion("HTTP/1.1");

                auto& headers = res->getHeaders();
                headers.set(Message::Headers::Standard::CONTENT_TYPE, "text/plain");
                headers.set(Message::Headers::Standard::SERVER, m_name);

                auto body = std::make_unique<StringBody>();
                std::string text = "404 Not Found\n";
                headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(text.size()));
                body->write(text.data(), text.size());
                res->setBody(std::move(body));
                return res;
            }

            if (std::filesystem::is_directory(filepath)) {
                auto res = std::make_unique<Response>();
                res->setStatusCode(Response::StatusCode::FORBIDDEN);
                res->setVersion("HTTP/1.1");

                auto& headers = res->getHeaders();
                headers.set(Message::Headers::Standard::CONTENT_TYPE, "text/plain");
                headers.set(Message::Headers::Standard::SERVER, m_name);

                auto body = std::make_unique<StringBody>();
                std::string text = "403 Forbidden: Directory listing not allowed\n";
                headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(text.size()));
                body->write(text.data(), text.size());
                res->setBody(std::move(body));
                return res;
            }

            auto filesize = std::filesystem::file_size(filepath);

            if (filesize == 0) {
                auto res = std::make_unique<Response>();
                res->setStatusCode(Response::StatusCode::NO_CONTENT);
                res->setVersion("HTTP/1.1");

                auto& headers = res->getHeaders();
                headers.set(Message::Headers::Standard::SERVER, m_name);

                return res;
            }

            auto res = std::make_unique<Response>();
            res->setStatusCode(Response::StatusCode::OK);
            res->setVersion("HTTP/1.1");

            auto& headers = res->getHeaders();

            auto mimeType = getMimeType(filepath);
            if (mimeType.starts_with("image/"))
            {
                std::cout << "Sending an image: " << filepath << std::endl;
            }
            if (mimeType.starts_with("text/"))
                headers.set(Message::Headers::Standard::CONTENT_TYPE, mimeType + "; charset=utf-8");
            else headers.set(Message::Headers::Standard::CONTENT_TYPE, mimeType);

            headers.set(Message::Headers::Standard::SERVER, m_name);
            headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(filesize));

            auto body = std::make_unique<FileBody>(filepath);
            res->setBody(std::move(body));

            return res;

            //auto ifModifiedSince = req->getHeaders()[http::field::if_modified_since];
            //if (!ifModifiedSince.empty()) {
            //    auto fileTime = std::filesystem::last_write_time(filepath);
            //    // TODO: Compare if-modified-since with file time
            //    // If not modified, return 304
            //    // This would require parsing the date string and comparing
            //}

            //res->setHeader(http::field::cache_control, "public, max-age=3600");

        }
        catch (const std::filesystem::filesystem_error& e) {
            // File system related errors
            auto res = std::make_unique<Response>();
            res->setStatusCode(Response::StatusCode::INTERNAL_SERVER_ERROR);
            res->setVersion("HTTP/1.1");

            auto& headers = res->getHeaders();
            headers.set(Message::Headers::Standard::SERVER, m_name);
            headers.set(Message::Headers::Standard::CONTENT_TYPE, "text/plain");

            auto body = std::make_unique<StringBody>();
            std::string text = "500 Internal Server Error: File system error\n";
            headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(text.size()));
            body->write(text.data(), text.size());
            res->setBody(std::move(body));

            return res;
        }
        catch (const std::exception& e) {
            auto res = std::make_unique<Response>();
            res->setStatusCode(Response::StatusCode::INTERNAL_SERVER_ERROR);
            res->setVersion("HTTP/1.1");

            auto& headers = res->getHeaders();
            headers.set(Message::Headers::Standard::SERVER, m_name);
            headers.set(Message::Headers::Standard::CONTENT_TYPE, "text/plain");

            auto body = std::make_unique<StringBody>();
            std::string text = "500 Internal Server Error\n";
            headers.set(Message::Headers::Standard::CONTENT_LENGTH, std::to_string(text.size()));
            body->write(text.data(), text.size());
            res->setBody(std::move(body));
            return res;
        }
    }

    std::unique_ptr<Response> Server::handleConnect(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handleDelete(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handleHead(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handleOptions(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handlePatch(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handlePost(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handlePut(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handleTrace(Request& req)
    {
        return nullptr;
    }

    std::unique_ptr<Response> Server::handleUnknown(Request& req)
    {
        return nullptr;
    }

}