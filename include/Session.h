#pragma once
#include "IOContext.h"
#include "Receiver.h"
#include "Sender.h"

namespace Network::HTTP
{

    //handles the loop, automatic session close and logging
    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        using BodyHandlerFunction = std::function<std::unique_ptr<Body>(std::unique_ptr<Message>&)>;
        using ResponseHandlerFunction = std::function<std::unique_ptr<Message>(std::unique_ptr<Message>&)>;

    private:
        Socket m_socket;
        ResponseHandlerFunction m_responseHandler;
        BodyHandlerFunction m_bodyHandler;
        std::string m_identifier;

        size_t m_bytesSent = 0;
        size_t m_bytesReceived = 0;
        size_t m_iterationCount = 0;
    public:
        Session(Socket&& socket, BodyHandlerFunction&& bodyHandler,
            ResponseHandlerFunction&& responseHandler, const std::string& identifier = "") :
            m_socket(std::move(socket)), m_bodyHandler(std::move(bodyHandler)),
            m_responseHandler(std::move(responseHandler)), m_identifier(identifier) {};

        ~Session() { m_socket.close(); };

        void start() {
            std::cout << "Session " << m_identifier << " started" << std::endl;
            if (!m_socket.waitForData(std::chrono::seconds(15)))
                return;

            while (true) {
                auto message = receiveMessage();
                if (message == nullptr)
                    break;

                std::cout << "Session " << m_identifier << " receiving: " << std::endl;
                std::cout << message->getFirstLine() << std::endl;

                auto response = m_responseHandler(message);

                std::cout << "Session " << m_identifier << " sending: " << std::endl;
                std::cout << response->getFirstLine() << std::endl;

                sendResponse(response);
                m_iterationCount++;

                if (message->getHeaders().get(Message::Headers::Standard::Connection) != "keep-alive"
                    || !m_socket.waitForData(std::chrono::seconds(15)))
                    break;
            }
            std::cout << "Session " << m_identifier << " ended" << std::endl;
        }

        void startAssync(IOContext& ioContext, IOContext::SessionCallback&& callback) {
            auto self = shared_from_this();
            ioContext.post([self, &ioContext, callback = std::move(callback)]() {
                self->start();
                ioContext.postSessionCallback(IOContext::SessionData{
                    self->m_bytesSent,self->m_bytesReceived,self->m_iterationCount }, std::move(callback));
                });
        }

        std::unique_ptr<Message> receiveMessage() {
            std::unique_ptr<Message> msg;
            std::string leftovers;
            m_bytesReceived += Receiver::read(m_socket, msg,
                [this](std::unique_ptr<Message>& message) ->std::unique_ptr<Body> {
                    return std::move(m_bodyHandler(message));
                });

            return msg;
        }

        void sendResponse(std::unique_ptr<Message>& res)
        {
            m_bytesSent += Sender::send(m_socket, res);
        }
    };

    //for user convenience
    using SessionPtr = std::shared_ptr<Session>;
}