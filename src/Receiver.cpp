#include "../include/Receiver.h"

namespace Network::HTTP
{

    void Receiver::parseFirstLine(std::stringstream& line, std::unique_ptr<Message>& message)
    {
        std::string token;

        if (!(line >> token)) {
            throw std::runtime_error("Invalid HTTP message: empty first line");
        }

        if (token.starts_with("HTTP/")) // message is a response
        {
            message = std::make_unique<Response>();
            auto* response = static_cast<Response*>(&(*message));

            // Validate and set version
            if (!token.starts_with("HTTP/1.") || token.length() != 8) {
                throw std::runtime_error("Invalid HTTP version: " + token);
            }
            response->setVersion(token);

            // Parse status code
            if (!(line >> token)) {
                throw std::runtime_error("Missing status code");
            }
            
            auto statusCode = Response::stringToStatusCode(token);
            if (statusCode == Response::StatusCode::Unknown)
                throw std::runtime_error("Unknown status code: " + token + "\n");
            response->setStatusCode(statusCode);

            // Get rest of line as status message (might contain spaces)
            std::string statusMsg;
            std::getline(line, statusMsg, '\n');
            if (!statusMsg.empty()) {
                if (statusMsg[0] == ' ') {
                    statusMsg = statusMsg.substr(1); // Remove leading space
                }
                // Remove trailing \r if present
                if (statusMsg.back() == '\r') {
                    statusMsg.pop_back();
                }
            }
            if (statusMsg.empty()) {
                throw std::runtime_error("Missing status message");
            }
            response->setStatusMessage(statusMsg);
        }
        else // message is a request
        {
            message = std::make_unique<Request>();
            auto* request = static_cast<Request*>(&(*message));

            // Set method
            auto method = Request::stringToMethod(token);
            if (method == Request::Method::Unknown)
                throw std::runtime_error("Unknown request method: " + token + "\n");
            request->setMethod(method);

            // Parse URI
            if (!(line >> token)) {
                throw std::runtime_error("Missing URI");
            }
            request->setUri(token);

            // Parse version
            if (!(line >> token)) {
                throw std::runtime_error("Missing HTTP version");
            }
            if (!token.starts_with("HTTP/1.") || token.length() != 8) {
                throw std::runtime_error("Invalid HTTP version: " + token);
            }
            request->setVersion(token);
            std::getline(line, token, '\n'); //remove end of line
        }
    }

    bool Receiver::parseHeaders(std::stringstream& headers, std::unique_ptr<Message>& message)
    {
        std::string header;
        std::string line;
        std::string value;
        auto& msgHeaders = message->getHeaders();

        while (headers.good() && !headers.eof())
        {
            // Read the next line
            std::getline(headers, line, '\n');

            // Check for empty line (marks end of headers)
            if (line == "\r" || line.empty())
                return true;

            // Find the colon
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos)
                throw std::runtime_error("Invalid header line (no colon): " + line);

            // Split into header and value
            header = line.substr(0, colonPos);
            value = line.substr(colonPos + 1);

            // Trim whitespace
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                value.erase(0, 1);

            // Remove trailing \r if present
            if (!value.empty() && value.back() == '\r')
                value.pop_back();

            // Validate header name and value
            if (header.empty() || value.empty())
                throw std::runtime_error("Invalid header: " + header + ":" + value);

            if (header.length() > s_maxHeaderNameLength)
                throw std::runtime_error("Header name too long: " + header);
            if (value.length() > s_maxHeaderValueLength)
                throw std::runtime_error("Header value too long: " + value);

            // Check header name doesn't contain invalid characters
            for (char c : header)
            {
                if (c <= 32 || c >= 127 || c == ':')  // No control chars, spaces, or colons
                    throw std::runtime_error("Invalid character in header name: " + header);
            }

            // Handle folded headers
            while (headers.good() && !headers.eof())
            {
                char next = headers.peek();
                if (next != ' ' && next != '\t')
                    break;

                // This is a folded header line
                std::string continuation;
                std::getline(headers, continuation, '\n');
                if (!continuation.empty() && continuation.back() == '\r')
                    continuation.pop_back();
                // Remove leading whitespace
                continuation.erase(0, continuation.find_first_not_of(" \t"));
                // Append to previous value with a space
                value += ' ' + continuation;

                if (value.length() > s_maxHeaderValueLength)
                    throw std::runtime_error("Header value too long after folding: " + header);
            }

            msgHeaders.set(header, value);
        }

        return false; // No empty line found - incomplete headers
    }

    std::pair<Message::TransferMethod, int> Receiver::determineTransferMethod(std::unique_ptr<Message>& message)
    {
        auto value = message->getHeaders().get(Message::Headers::Standard::TransferEncoding);
        if (!value.empty()) {
            if (value == "chunked") {
                return std::make_pair<Message::TransferMethod, int>
                    (Message::TransferMethod::Chunked, 0);
            }

            // Other transfer encodings are not supported in HTTP/1.1
            throw std::runtime_error("Unsupported Transfer-Encoding: " + value);
        }
        value = message->getHeaders().get(Message::Headers::Standard::ContentLength);

        if (!value.empty() && value != "0") {
            try {
                size_t length = std::stoull(value);
                return std::make_pair<Message::TransferMethod, int>
                    (Message::TransferMethod::ContentLength, length);
            }
            catch (const std::exception&) {
                throw std::runtime_error("Invalid Content-Length: " + value);
            }
        }

        return std::make_pair<Message::TransferMethod, int>
            (Message::TransferMethod::None, 0);
    }

    size_t Receiver::readHeader(Socket& sock, Buffer& leftovers, std::unique_ptr<Message>& message)
    {
        leftovers.resize(1024);
        size_t bytesReadTotal = 0;

        try
        {
            sock.receiveLoop(leftovers.data(),
                leftovers.size(), 0, s_maxRetryCount,
                [&leftovers, &message, &bytesReadTotal]
                (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {

                    if (receivedTotal > s_maxHeaderSize) {
                        throw std::runtime_error("HTTP header too large (exceeds "
                            + std::to_string(s_maxHeaderSize / 1024) + "KB limit, received: "
                            + std::to_string(receivedTotal / 1024) + "KB)");
                    }

                    auto headerEnd = leftovers.find("\r\n\r\n");
                    if (headerEnd == std::string::npos) {

                        if (leftovers.size() - receivedTotal < leftovers.size() / 4)
                            leftovers.resize(leftovers.size() * 2);
                        len = leftovers.size() - receivedTotal;
                        buffer = leftovers.data() + receivedTotal;
                        return true;
                    }

                    bytesReadTotal = receivedTotal;
                    std::stringstream ss(leftovers.substr(0, headerEnd + 4));
                    leftovers.erase(0, headerEnd + 4);
                    leftovers.resize(bytesReadTotal - headerEnd - 4);

                    parseFirstLine(ss, message);
                    parseHeaders(ss, message);
                    return false;
                }
            );
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
            return 0;
        }
        return bytesReadTotal;
    }

    size_t Receiver::read(Socket& sock, std::unique_ptr<Message>& message)
    {
        std::string leftovers;
        size_t bytesRead = 0;
        try
        {
            std::unique_ptr<Message> message;
            bytesRead += readHeader(sock, leftovers, message);
            std::unique_ptr<StringBody> body = std::make_unique<StringBody>();

            bytesRead += readBody<StringBody>(sock, leftovers, message);

            return bytesRead;
        }
        catch (std::exception& e)
        {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        }
    }

    size_t Receiver::read(Socket& sock, std::unique_ptr<Message>& message, BodyTypeHandler handler)
    {
        std::string leftovers;
        size_t bytesRead = 0;

        try
        {
            bytesRead += readHeader(sock, leftovers, message);
            if (message == nullptr)
                return 0;

            auto methodAndLength = determineTransferMethod(message);
            message->setBody(handler(message));

            switch (methodAndLength.first)
            {
            case Message::TransferMethod::ContentLength:
                bytesRead += message->getBody()->readTransferSize
                (sock, leftovers, methodAndLength.second, s_maxRetryCount, s_maxBodySize);
                break;
            case Message::TransferMethod::Chunked:
                bytesRead += message->getBody()->readChunked
                (sock, leftovers, s_maxRetryCount, s_maxBodySize);
                break;
            default:
                break; //HTTP/1.1 only supports chunked or content length transfer methods
            }

            return bytesRead;
        }
        catch (std::exception& e)
        {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        }
    }


    void Receiver::asyncRead(IOContext& context,Socket& sock,
        std::unique_ptr<Message>& message, std::function<void(size_t)> callback)
    {
        try
        {
            context.post([&context, &sock, &message, callback]() {
                try
                {
                    size_t bytesRead = 0;
                    Buffer leftovers;
                    bytesRead += readHeader(sock, leftovers, message);

                    bytesRead += readBody<StringBody>(sock, leftovers, message);

                    context.postParserCallback(bytesRead, callback);
                }
                catch (const std::exception& e)
                {
                    context.postParserCallback(0, callback);
                }
                });
        }
        catch (std::exception e)
        {
            throw std::runtime_error("Error during asynchronous header reading: " + std::string(e.what()));
        }
    }

    void Receiver::asyncRead(IOContext& context, Socket& sock,
        std::unique_ptr<Message>& message, BodyTypeHandler handler,
        std::function<void(size_t)> callback)
    {
        try
        {
            context.post([&context, &sock, &message, handler, callback]() {
                size_t bytesRead = 0;
                Buffer leftovers;
                std::unique_ptr<Message> message;

                try
                {
                    bytesRead += readHeader(sock, leftovers, message);
                    auto methodAndLength = determineTransferMethod(message);
                    message->setBody(handler(message));

                    switch (methodAndLength.first)
                    {
                    case Message::TransferMethod::ContentLength:
                        bytesRead += message->getBody()->readTransferSize
                        (sock, leftovers, methodAndLength.second, s_maxRetryCount, s_maxBodySize);
                        break;
                    case Message::TransferMethod::Chunked:
                        bytesRead += message->getBody()->readChunked
                        (sock, leftovers, s_maxRetryCount, s_maxBodySize);
                        break;
                    default:
                        break; //HTTP/1.1 only supports chunked or content length transfer methods
                    }

                    context.postParserCallback(bytesRead, callback);
                }
                catch (const std::exception& e)
                {
                    context.postParserCallback(0, callback);
                }
                });
        }
        catch (std::exception e)
        {
            throw std::runtime_error("Error during asynchronous header reading: " + std::string(e.what()));
        }
    }

    void Receiver::asyncReadHeader(IOContext& context, Socket& sock,
        Buffer& leftovers, std::unique_ptr<Message>& message,
        std::function<void(size_t)> callback)
    {
        try
        {
            context.post([&context, &sock, callback, &leftovers, &message]() {
                try
                {
                    size_t bytesRead = 0;
                    bytesRead = readHeader(sock, leftovers, message);
                    context.postParserCallback(bytesRead, callback);
                }
                catch (const std::exception& e)
                {
                    context.postParserCallback(0, callback);
                }
                });
        }
        catch (std::exception e)
        {
            throw std::runtime_error("Error during asynchronous header reading: " + std::string(e.what()));
        }
    }
}