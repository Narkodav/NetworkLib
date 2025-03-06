#include "Parser.h"


namespace http
{


    std::unique_ptr<Message> Parser::parseFirstLine(std::stringstream& line)
    {
        std::string token;

        if (!(line >> token)) {
            throw std::runtime_error("Invalid HTTP message: empty first line");
        }

        if (token.starts_with("HTTP/")) // message is a response
        {
            auto response = std::make_unique<Response>();

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
            if (statusCode == Response::StatusCode::UNKNOWN)
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

            return response;
        }
        else // message is a request
        {
            auto request = std::make_unique<Request>();

            // Set method
            auto method = Request::stringToMethod(token);
            if (method == Request::Method::UNKNOWN)
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

            return request;
        }
    }

    bool Parser::parseHeaders(std::stringstream& headers, std::unique_ptr<Message>& message)
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

            if (header.length() > MAX_HEADER_NAME_LENGTH)
                throw std::runtime_error("Header name too long: " + header);
            if (value.length() > MAX_HEADER_VALUE_LENGTH)
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

                if (value.length() > MAX_HEADER_VALUE_LENGTH)
                    throw std::runtime_error("Header value too long after folding: " + header);
            }

            msgHeaders.set(header, value);
        }

        return false; // No empty line found - incomplete headers
    }

    std::pair<Message::TransferMethod, int> Parser::determineTransferMethod(std::unique_ptr<Message>& message)
    {
        auto value = message->getHeaders().get(Message::Headers::Standard::TRANSFER_ENCODING);
        if (!value.empty()) {
            if (value == "chunked") {
                return std::make_pair<Message::TransferMethod, int>
                    (Message::TransferMethod::CHUNKED, 0);
            }

            // Other transfer encodings are not supported in HTTP/1.1
            throw std::runtime_error("Unsupported Transfer-Encoding: " + value);
        }
        value = message->getHeaders().get(Message::Headers::Standard::CONTENT_LENGTH);

        if (!value.empty()) {
            try {
                size_t length = std::stoull(value);
                return std::make_pair<Message::TransferMethod, int>
                    (Message::TransferMethod::CONTENT_LENGTH, length);
            }
            catch (const std::exception&) {
                throw std::runtime_error("Invalid Content-Length: " + value);
            }
        }

        return std::make_pair<Message::TransferMethod, int>
            (Message::TransferMethod::NONE, 0);
    }

    //void Parser::parseBodyChunked(Socket& sock, Buffer& leftovers,
    //    std::unique_ptr<Message>& message) //TODO
    //{
    //    //std::vector<char>& body = body;
    //    //std::vector<char> tempBuffer(1024); // Small buffer for incoming data
    //    //size_t bodySize = 0;  // Track actual data size in main buffer

    //    //// First handle any existing data in the buffer from header parsing
    //    //if (!body.empty()) {
    //    //    size_t currentPos = 0;
    //    //    while (currentPos < body.size()) {
    //    //        // Find next chunk size line
    //    //        auto lineEnd = std::search(body.begin() + currentPos,
    //    //            body.end(),
    //    //            "\r\n", "\r\n" + 2);

    //    //        if (lineEnd == body.end()) {
    //    //            // Incomplete chunk, move remaining data to start of buffer
    //    //            std::memmove(body.data(),
    //    //                body.data() + currentPos,
    //    //                body.size() - currentPos);
    //    //            body.resize(body.size() - currentPos);
    //    //            break;
    //    //        }

    //    //        // Parse chunk size
    //    //        std::string sizeStr(body.begin() + currentPos, lineEnd);
    //    //        size_t chunkSize = std::stoull(sizeStr, nullptr, 16);

    //    //        // Move past chunk size line
    //    //        currentPos = (lineEnd - body.begin()) + 2;

    //    //        // Check if we have the full chunk
    //    //        if (currentPos + chunkSize + 2 > body.size()) {
    //    //            // Incomplete chunk, move remaining data to start
    //    //            std::memmove(body.data(),
    //    //                body.data() + currentPos - 2,
    //    //                body.size() - (currentPos - 2));
    //    //            body.resize(body.size() - (currentPos - 2));
    //    //            break;
    //    //        }

    //    //        // Move chunk data to final position
    //    //        std::memmove(body.data() + bodySize,
    //    //            body.data() + currentPos,
    //    //            chunkSize);
    //    //        bodySize += chunkSize;

    //    //        // Skip chunk data and its CRLF
    //    //        currentPos += chunkSize + 2;
    //    //    }
    //    //}

    //    //// Now continue reading remaining chunks
    //    //while (true) {
    //    //    // Find the chunk size line ending in existing data
    //    //    auto dataEnd = body.begin() + bodySize;
    //    //    auto crlfPos = std::search(dataEnd, body.end(),
    //    //        "\r\n", "\r\n" + 2);

    //    //    // If we don't have a complete line, receive more
    //    //    while (crlfPos == body.end()) {
    //    //        auto bytesRead = m_socket.receive(tempBuffer.data(), tempBuffer.size());
    //    //        if (bytesRead <= 0) {
    //    //            throw std::runtime_error("Connection closed while reading chunk size");
    //    //        }

    //    //        // Append to main buffer
    //    //        body.insert(body.end(),
    //    //            tempBuffer.begin(),
    //    //            tempBuffer.begin() + bytesRead);
    //    //        crlfPos = std::search(dataEnd, body.end(),
    //    //            "\r\n", "\r\n" + 2);
    //    //    }

    //    //    // Parse chunk size
    //    //    std::string sizeStr(dataEnd, crlfPos);
    //    //    size_t chunkSize = std::stoull(sizeStr, nullptr, 16);

    //    //    // If chunk size is 0, we're done
    //    //    if (chunkSize == 0) {
    //    //        // Remove the final chunk marker and CRLF
    //    //        body.resize(bodySize);
    //    //        break;
    //    //    }

    //    //    // Move existing chunk data to the correct position (if any)
    //    //    size_t availableData = body.end() - (crlfPos + 2);
    //    //    size_t dataToMove = min(availableData, chunkSize);
    //    //    if (dataToMove > 0) {
    //    //        std::memmove(body.data() + bodySize,
    //    //            crlfPos + 2,
    //    //            dataToMove);
    //    //    }

    //    //    // Read remaining chunk data if needed
    //    //    size_t remainingBytes = chunkSize - dataToMove;
    //    //    while (remainingBytes > 0) {
    //    //        auto bytesRead = m_socket.receive(tempBuffer.data(),
    //    //            min(remainingBytes, tempBuffer.size()));
    //    //        if (bytesRead <= 0) {
    //    //            throw std::runtime_error("Connection closed while reading chunk data");
    //    //        }

    //    //        // Copy directly to the correct position in main buffer
    //    //        std::memcpy(body.data() + bodySize + dataToMove,
    //    //            tempBuffer.data(),
    //    //            bytesRead);
    //    //        remainingBytes -= bytesRead;
    //    //        dataToMove += bytesRead;
    //    //    }

    //    //    bodySize += chunkSize;

    //    //    // Read and discard chunk trailing CRLF
    //    //    char crlf[2];
    //    //    size_t crlfRead = 0;
    //    //    while (crlfRead < 2) {
    //    //        auto result = m_socket.receive(crlf + crlfRead, 2 - crlfRead);
    //    //        if (result <= 0) {
    //    //            throw std::runtime_error("Connection closed while reading chunk CRLF");
    //    //        }
    //    //        crlfRead += result;
    //    //    }
    //    //}

    //    //// Resize the buffer to the actual data size
    //    //body.resize(bodySize);
    //}

    //void Parser::parseBodyTransferSize(Socket& sock, Buffer& leftovers,
    //    size_t contentLength, std::unique_ptr<Message>& message)
    //{
    //    std::vector<char> buffer(contentLength);
    //    size_t retryCount = 0;
    //    size_t bytesRead = buffer.size();

    //    while (bytesRead < contentLength) {
    //        auto result = m_socket.receive(body.data() + bytesRead, contentLength - bytesRead);
    //        if (bytesRead == 0) {
    //            std::cout << "Connection closed by client" << std::endl;
    //            break;
    //        }
    //        else if (bytesRead < 0) {
    //            auto error = Socket::getLastError();
    //            if (error == Socket::Error::INTERRUPTED ||
    //                error == Socket::Error::WOULD_BLOCK) {
    //                if (++retryCount > MAX_RETRY_COUNT) {
    //                    std::cerr << "Max retries exceeded" << std::endl;
    //                    break;
    //                }
    //                // Exponential backoff: 10ms, 20ms, 40ms, 80ms, 160ms
    //                std::this_thread::sleep_for(
    //                    std::chrono::milliseconds(10 * (1 << (retryCount - 1)))
    //                );
    //                continue;
    //            }
    //            else {
    //                std::cerr << "Error reading from socket: " << Socket::getErrorString(error) << std::endl;
    //                break;
    //            }
    //        }
    //        bytesRead += result;
    //    }
    //}

    std::unique_ptr<Message> Parser::parseHeader(Socket& sock, Buffer& leftovers)
    {
        leftovers.reserve(1024);
        std::unique_ptr<Message> message;

        try
        {
            sock.receiveLoop(leftovers.data(),
                leftovers.size(), 0, MAX_RETRY_COUNT,
                [this, &leftovers, &message]
                (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {

                    if (receivedTotal > MAX_HEADER_SIZE) {
                        throw std::runtime_error("HTTP header too large (exceeds "
                            + std::to_string(MAX_HEADER_SIZE / 1024) + "KB limit, received: "
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

                    std::stringstream ss(leftovers.substr(0, headerEnd + 4));
                    leftovers.erase(0, headerEnd + 4);
                    leftovers.shrink_to_fit();
                    message = parseFirstLine(ss);
                    parseHeaders(ss, message);
                    return false;
                }
            );
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
            return nullptr;
        }
        return message;
    }


    void Parser::parseBody(Socket& sock, std::unique_ptr<Message>& message,
        Buffer& leftovers, std::unique_ptr<Message::Body>&& bodyStorage)
    {
        std::pair<Message::TransferMethod, int> methodAndLength =
            determineTransferMethod(message);
        message->setBody(std::move(bodyStorage));

        switch (methodAndLength.first)
        {
        case Message::TransferMethod::CONTENT_LENGTH:
            message->getBody()->parseTransferSize(sock, leftovers, methodAndLength.second);
            break;
        case Message::TransferMethod::CHUNKED:
            message->getBody()->parseChunked(sock, leftovers);
            break;
        default:
            break; //HTTP/1.1 only supports chunked or content length transfer methods
        }
    }

}