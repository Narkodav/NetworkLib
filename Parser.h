#pragma once
#include "Socket.h"
#include "IOContext.h"
#include "Message.h"

// takes a client socket a buffer and a io context returns an http request
// doesn't actually store anything or have a state so its static

namespace http
{
    class Parser
    {
    public:
        using Buffer = std::string;

        static inline const size_t MAX_RETRY_COUNT = 5;
        static inline const size_t MAX_BODY_SIZE = 1024 * 1024 * 16;
        static inline const size_t MAX_HEADER_SIZE = 1024 * 16; //16 KBs
        static inline const size_t MAX_HEADER_NAME_LENGTH = 256;
        static inline const size_t MAX_HEADER_VALUE_LENGTH = 8192;

    private:

        std::unique_ptr<Message> parseFirstLine(std::stringstream& line); //returns a message of the needed type
        bool parseHeaders(std::stringstream& headers, std::unique_ptr<Message>& message);

        //stores bytes if used content length
        std::pair<Message::TransferMethod, int> determineTransferMethod(std::unique_ptr<Message>& message);

    public:


        std::unique_ptr<Message> parseHeader(Socket& sock, Buffer& leftovers); //buffer will store leftovers

        // bodyStorage will be moved to message and filled with body data (if any is present)
        void parseBody(Socket& sock, std::unique_ptr<Message>& message,
            Buffer& leftovers, std::unique_ptr<Message::Body>&& bodyStorage); 

        // parse the message completely
        std::unique_ptr<Message> parseMessage(Socket& sock);

        //std::unique_ptr<Message> asyncParse(IOContext& ioContext, Socket& sock,
        //    Buffer& buffer, std::function<void(Socket&&)> callback);
    };
}