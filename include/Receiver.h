#pragma once
#include "Socket.h"
#include "IOContext.h"
#include "Message.h"

// takes a client socket a buffer and a io context returns an http request
// doesn't actually store anything or have a state so its static

namespace Network::HTTP
{
    class Receiver
    {
    public:
        using Buffer = std::string;
        using BodyTypeHandler = std::function<std::unique_ptr<Body>(std::unique_ptr<Message>&)>;

    private:

        static void parseFirstLine(std::stringstream& line, std::unique_ptr<Message>& message);
        static bool parseHeaders(std::stringstream& headers, std::unique_ptr<Message>& message);

        //stores bytes if used content length
        static std::pair<Message::TransferMethod, int> determineTransferMethod(std::unique_ptr<Message>& message);

    public:


        static size_t readHeader(Socket& sock, Buffer& leftovers, std::unique_ptr<Message>& message); //buffer will store leftovers

        template<typename BodyType>
        static size_t readBody(Socket& sock, Buffer& leftovers,
            std::unique_ptr<Message>& message)
        {
            std::pair<Message::TransferMethod, int> methodAndLength =
                determineTransferMethod(message);
            message->setBody(std::move(std::make_unique<BodyType>()));

            switch (methodAndLength.first)
            {
            case Message::TransferMethod::ContentLength:
                return message->getBody()->readTransferSize
                (sock, leftovers, methodAndLength.second, s_maxRetryCount, s_maxBodySize);
                break;
            case Message::TransferMethod::Chunked:
                return message->getBody()->readChunked
                (sock, leftovers, s_maxRetryCount, s_maxBodySize);
                break;
            default:
                break; //HTTP/1.1 only supports chunked or content length transfer methods
            }
        }

        // parse the message completely
        static size_t read(Socket& sock, std::unique_ptr<Message>& message);
        static size_t read(Socket& sock, std::unique_ptr<Message>& message, BodyTypeHandler handler);

        static void asyncRead(IOContext& context, Socket& sock,
            std::unique_ptr<Message>& message, std::function<void(size_t)> callback);

        static void asyncRead(IOContext& context, Socket& sock,
            std::unique_ptr<Message>& message, BodyTypeHandler handler,
            std::function<void(size_t)> callback);

        static void asyncReadHeader(IOContext& context, Socket& sock,
            Buffer& leftovers, std::unique_ptr<Message>& message,
            std::function<void(size_t)> callback);

        template<typename BodyType>
        static void asyncReadBody(IOContext& context, Socket& sock,
            Buffer& leftovers, std::unique_ptr<Message>& message, std::function<void(size_t)> callback)
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