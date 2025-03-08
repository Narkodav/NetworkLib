#pragma once
#include "IOContext.h"
#include "Receiver.h"
#include "Sender.h"

namespace http
{

    //handles the loop, automatic session close and logging
    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        using ResponseHandlerFunction = std::function<std::unique_ptr<Message>(std::unique_ptr<Message>&)>;
        using BodyHandlerFunction = std::function<std::unique_ptr<Body>(std::unique_ptr<Message>&)>;

    private:
        Socket m_socket;
        ResponseHandlerFunction m_responseHandler;
        BodyHandlerFunction m_bodyHandler;

        size_t m_bytesSent = 0;
        size_t m_bytesReceived = 0;
        size_t m_iterationCount = 0;
    public:
        Session(Socket&& socket, BodyHandlerFunction&& bodyHandler, ResponseHandlerFunction&& responseHandler) :
            m_socket(std::move(socket)), m_bodyHandler(std::move(bodyHandler)),
            m_responseHandler(std::move(responseHandler)) {};

        ~Session() { m_socket.close(); };

        void start() {
            while (true) {
                auto message = receiveMessage();
                auto response = m_responseHandler(message);
                sendResponse(response);
                m_iterationCount++;

                if (message->getHeaders().get(Message::Headers::Standard::CONNECTION) != "keep-alive"
                    || !m_socket.waitForData(std::chrono::seconds(15)))
                    break;
            }
        }

        void startAssync(IOContext& ioContext, IOContext::SessionCallback&& callback) {
            auto self = shared_from_this();
            ioContext.post([self, &ioContext, callback = std::move(callback)]() {
                self->start();
                ioContext.postSessionCallback(IOContext::SessionData{
                    self->m_bytesSent,self->m_bytesReceived,self->m_iterationCount }, std::move(callback));
                });
        }

        MessagePtr receiveMessage() {
            MessagePtr msg;
            std::string leftovers;
            m_bytesReceived += Receiver::read(m_socket, msg,
                [this](MessagePtr& message) ->std::unique_ptr<Body> {
                    return std::move(m_bodyHandler(message));
                });

            return msg;
        }

        void sendResponse(MessagePtr& res)
        {
            m_bytesSent += Sender::send(m_socket, res);
        }
    };

    //for user convenience
    using SessionPtr = std::shared_ptr<Session>;
}