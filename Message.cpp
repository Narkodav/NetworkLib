#include "Message.h"

namespace http
{
    void Message::StringBody::parseTransferSize(Socket& sock,
        std::string& leftovers, size_t size, size_t maxRetryCount,
        size_t maxBodySize)
	{
        if (size > maxBodySize) {
            throw std::runtime_error("Content size exceeds maximum allowed ("
                + std::to_string(size / 1024) + "KB > "
                + std::to_string(maxBodySize / 1024) + "KB)");
        }

        if (!leftovers.empty())
        {
            if (leftovers.size() > size) {
                throw std::runtime_error("Leftover data size exceeds expected content size");
            }
            m_data = leftovers;
        }

        m_data.resize(size);

        try
        {
            sock.receiveLoop(m_data.data() + leftovers.size(),
                m_data.size() - leftovers.size(), leftovers.size(), maxRetryCount,
                [this]
                (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {
                    if (receivedTotal > m_data.size()) {  // Add overflow check
                        throw std::runtime_error("Received more data than expected");
                    }
                    if (receivedTotal == m_data.size())
                        return false;
                    buffer = m_data.data() + receivedTotal;
                    len = m_data.size() - receivedTotal;
                    return true;
                }
            );
        }
        catch (const std::exception& e)
        {
            m_data.clear();
            throw std::runtime_error(std::string("Body parse error: ") + e.what());
        }
	}

    void Message::StringBody::parseChunked(Socket& sock, std::string& leftovers,
        size_t maxRetryCount, size_t maxBodySize)
	{
        size_t chunksReceived = 0;
        size_t chunkSize = 0;
        size_t chunkStart = 0;
        size_t displacementCounter= 0;
        size_t cursor = 0;
        bool isInChunk = false;
        bool isParsingHex = false;
        std::string hexBuffer;

        if (!leftovers.empty())
            m_data = leftovers;

        m_data.resize(1024);

        try
        {
            sock.receiveLoop(m_data.data() + leftovers.size(),
                m_data.size() - leftovers.size(), leftovers.size(), maxRetryCount,
                [this, &maxBodySize, &cursor, &displacementCounter, &chunkStart,
                &isInChunk, &hexBuffer, &isParsingHex, &chunkSize, &chunksReceived]
                (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {
                    for (cursor; cursor < receivedTotal; cursor++)
                    {
                        if (isParsingHex)
                        {
                            if (m_data[cursor] != '\r')
                                hexBuffer += m_data[cursor];
                            else
                            {
                                isParsingHex = false;
                                cursor += 1;
                                chunkSize = hexToDec(hexBuffer);
                                displacementCounter += hexBuffer.size() + 4;
                                hexBuffer.clear();
                                if (chunkSize == 0)
                                {
                                    m_data.resize(receivedTotal - displacementCounter);
                                    return false;
                                }
                                chunksReceived++;
                                chunkStart = cursor + 1;
                            }
                        }
                        else if (m_data[cursor] == '\r')
                        {
                            if (cursor - chunkStart - 1 != chunkSize)
                                throw std::runtime_error("Chunk size: " + std::to_string(cursor - chunkStart - 1) +
                                    " doesn't match expected size: " + std::to_string(chunkSize));
                            cursor += 1;
                            isParsingHex = true;
                        }
                        else
                        {
                            m_data[cursor - displacementCounter] = m_data[cursor];
                        }
                    }

                    receivedTotal -= displacementCounter;
                    displacementCounter = 0;

                    if (receivedTotal > maxBodySize) {
                        throw std::runtime_error("Content size exceeds maximum allowed ("
                            + std::to_string(receivedTotal / 1024) + "KB > "
                            + std::to_string(maxBodySize / 1024) + "KB)");
                    }
                    
                    if (m_data.size() - receivedTotal < 256) {  // Getting low on space
                        m_data.resize(m_data.size() * 2);
                    }

                    buffer = m_data.data() + receivedTotal;
                    len = m_data.size() - receivedTotal;
                    return true;
                }
            );
        }
        catch (const std::exception& e)
        {
            m_data.clear();
            throw std::runtime_error(std::string("Body parse error: ") + e.what());
        }
	}

	void Message::FileBody::parseTransferSize(Socket& sock,
        std::string& leftovers, size_t size, size_t maxRetryCount,
        size_t maxBodySize)
	{


	}

    void Message::FileBody::parseChunked(Socket& sock, std::string& leftovers,
        size_t maxRetryCount, size_t maxBodySize)
	{


	}
}