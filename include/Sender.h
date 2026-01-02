#pragma once
#include "Socket.h"
#include "IOContext.h"
#include "Message.h"

namespace Network::HTTP
{
    class Sender
    {   
    public:

        static size_t sendHeaders(Socket& sock, std::unique_ptr<Message>& message);
        static size_t sendBody(Socket& sock, std::unique_ptr<Message>& message);
        static size_t send(Socket& sock, std::unique_ptr<Message>& message);

    };

}
