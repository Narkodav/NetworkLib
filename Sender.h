#pragma once
#include "Socket.h"
#include "IOContext.h"
#include "Message.h"

namespace http
{
    class Sender
    {   
    public:

        static size_t sendHeaders(Socket& sock, MessagePtr& message);
        static size_t sendBody(Socket& sock, MessagePtr& message);
        static size_t send(Socket& sock, MessagePtr& message);

    };

}
