#pragma once
#include "IOContext.h"
#include "Parser.h"

namespace http
{
    class Session : public std::enable_shared_from_this<Session>
    {
    private:
        Socket socket;
        bool keepAlive = true;

    public:
        Session(Socket&& socket) : socket(std::move(socket)) {};

        void start() {
            while (keepAlive) {
                auto message = receiveMessage();
                auto response = handleMessage(message);
                sendResponse(response);

                keep_alive = message.keepAlive() && !socket.isClosed();
            }
        }

        MessagePtr receiveMessage() {
            MessagePtr msg;
            std::string leftovers;
            size_t bytesReceived = 0;
            bytesReceived += Parser::readHeader(socket, leftovers, msg);

            // determine body type and read body
            bytesReceived += Parser::readBody<StringBody>(socket, leftovers, msg);

            return msg;
        }

        MessagePtr handleMessage(MessagePtr& msg) {
            MessagePtr rsp;

            //handling message

            return rsp;
        }
    };

    //for user convenience
    using SessionPtr = std::shared_ptr<Session>;
}