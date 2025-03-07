#pragma once
#include "Socket.h"
#include "IOContext.h"
#include "Message.h"

// takes a client socket a buffer and a io context returns an http request
// doesn't actually store anything or have a state so its static

namespace http
{
    class Receiver
    {
    public:
        using Buffer = std::string;
        using BodyTypeHandler = std::function<std::unique_ptr<Body>(MessagePtr&)>;

    private:

        static MessagePtr parseFirstLine(std::stringstream& line); //returns a message of the needed type
        static bool parseHeaders(std::stringstream& headers, MessagePtr& message);

        //stores bytes if used content length
        static std::pair<Message::TransferMethod, int> determineTransferMethod(MessagePtr& message);

    public:


        static size_t readHeader(Socket& sock, Buffer& leftovers, MessagePtr& message); //buffer will store leftovers

        template<typename BodyType>
        static size_t readBody(Socket& sock, Buffer& leftovers,
            MessagePtr& message)
        {
            std::pair<Message::TransferMethod, int> methodAndLength =
                determineTransferMethod(message);
            message->setBody(std::move(std::make_unique<BodyType>()));

            switch (methodAndLength.first)
            {
            case Message::TransferMethod::CONTENT_LENGTH:
                return message->getBody()->parseTransferSize
                (sock, leftovers, methodAndLength.second, MAX_RETRY_COUNT, MAX_BODY_SIZE);
                break;
            case Message::TransferMethod::CHUNKED:
                return message->getBody()->parseChunked
                (sock, leftovers, MAX_RETRY_COUNT, MAX_BODY_SIZE);
                break;
            default:
                break; //HTTP/1.1 only supports chunked or content length transfer methods
            }
        }

        // parse the message completely
        static size_t read(Socket& sock, MessagePtr& message);
        static size_t read(Socket& sock, MessagePtr& message, BodyTypeHandler handler);

        static void asyncRead(IOContext& context, Socket& sock,
            MessagePtr& message, std::function<void(size_t)> callback);

        static void asyncRead(IOContext& context, Socket& sock,
            MessagePtr& message, BodyTypeHandler handler,
            std::function<void(size_t)> callback);

        static void asyncReadHeader(IOContext& context, Socket& sock,
            Buffer& leftovers, MessagePtr& message,
            std::function<void(size_t)> callback);

        template<typename BodyType>
        static void asyncReadBody(IOContext& context, Socket& sock,
            Buffer& leftovers, MessagePtr& message, std::function<void(size_t)> callback)
        {
            try
            {
                context.post([&context, &sock, callback, &leftovers, &message]() {
                    try
                    {
                        size_t bytesRead = 0;
                        bytesRead = readBody(sock, leftovers, message);
                        context.postParserCallback(bytesRead, callback);
                    }
                    catch (const std::exception& e)
                    {

                    }
                    });
            }
            catch (std::exception e)
            {
                throw std::runtime_error("Error during asynchronous header reading: " + std::string(e.what()));
            }
        }
    };
}