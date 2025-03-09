#include "Body.h"

namespace http
{
    size_t StringBody::readTransferSize(Socket& sock,
        std::string& leftovers, size_t size, size_t maxRetryCount,
        size_t maxBodySize)
    {
        size_t bytesRead = leftovers.size() - size;

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

        return bytesRead;
    }

    size_t StringBody::readChunked(Socket& sock, std::string& leftovers,
        size_t maxRetryCount, size_t maxBodySize)
    {
        size_t bytesInitial = leftovers.size();

        size_t chunksReceived = 0;
        size_t chunkSize = 0;
        size_t chunkStart = 0;
        size_t displacementCounter = 0;
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
                                    receivedTotal -= displacementCounter;
                                    m_data.resize(receivedTotal);
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
                    cursor -= displacementCounter;
                    displacementCounter = 0;

                    if (receivedTotal > maxBodySize) {
                        throw std::runtime_error("Content size exceeds maximum allowed ("
                            + std::to_string(receivedTotal / 1024) + "KB > "
                            + std::to_string(maxBodySize / 1024) + "KB)");
                    }

                    if (m_data.size() - receivedTotal < 256) {
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

        return m_data.size() - bytesInitial;
    }

    size_t FileBody::readTransferSize(Socket& sock,
        std::string& leftovers, size_t size, size_t maxRetryCount,
        size_t maxBodySize)
    {
        size_t bytesRead = leftovers.size() - size;

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
            m_file << leftovers;
        }

        m_size = leftovers.size();
        leftovers.resize(1024);

        try
        {
            m_size = sock.receiveLoop(leftovers.data(),
                leftovers.size(), m_size, maxRetryCount,
                [this, &size, &leftovers]
                (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {
                    if (receivedTotal > size) {  // Add overflow check
                        throw std::runtime_error("Received more data than expected");
                    }
                    if (receivedTotal == size)
                        return false;

                    m_file << leftovers.substr(0, bytesRead);
                    buffer = leftovers.data();
                    len = leftovers.size();
                    return true;
                }
            );


        }
        catch (const std::exception& e)
        {
            m_file.clear();
            throw std::runtime_error(std::string("Body parse error: ") + e.what());
        }

        return bytesRead;
    }

    size_t FileBody::readChunked(Socket& sock, std::string& leftovers,
        size_t maxRetryCount, size_t maxBodySize)
    {

        size_t chunksReceived = 0;
        size_t chunkSize = 0;
        size_t chunkStart = 0;
        size_t displacementCounter = 0;
        size_t cursor = 0;
        bool isInChunk = false;
        bool isParsingHex = false;
        std::string hexBuffer;
        size_t initSize;

        initSize = leftovers.size();
        leftovers.resize(1024);
        m_file.seekp(0, std::ios::end);

        try
        {
            sock.receiveLoop(leftovers.data() + initSize,
                leftovers.size() - initSize, initSize, maxRetryCount,
                [this, &maxBodySize, &cursor, &displacementCounter, &chunkStart,
                &isInChunk, &hexBuffer, &isParsingHex, &chunkSize, &chunksReceived,
                &leftovers, &initSize]
                (char*& buffer, size_t& len, size_t bytesRead, size_t& receivedTotal) {
                    for (cursor; cursor < receivedTotal; cursor++)
                    {
                        if (isParsingHex)
                        {
                            if (leftovers[cursor] != '\r')
                                hexBuffer += leftovers[cursor];
                            else
                            {
                                isParsingHex = false;
                                cursor += 1;
                                chunkSize = hexToDec(hexBuffer);
                                displacementCounter += hexBuffer.size() + 4;
                                hexBuffer.clear();
                                if (chunkSize == 0)
                                {
                                    receivedTotal -= displacementCounter;
                                    cursor -= displacementCounter;
                                    m_file << leftovers.substr(0, cursor);
                                    initSize = receivedTotal;
                                    return false;
                                }
                                chunksReceived++;
                                chunkStart = cursor + 1;
                            }
                        }
                        else if (leftovers[cursor] == '\r')
                        {
                            if (cursor - chunkStart - 1 != chunkSize)
                                throw std::runtime_error("Chunk size: " + std::to_string(cursor - chunkStart - 1) +
                                    " doesn't match expected size: " + std::to_string(chunkSize));
                            cursor += 1;
                            isParsingHex = true;
                        }
                        else
                        {
                            leftovers[cursor - displacementCounter] = leftovers[cursor];
                        }
                    }

                    receivedTotal -= displacementCounter;
                    cursor -= displacementCounter;
                    displacementCounter = 0;

                    if (receivedTotal > maxBodySize) {
                        throw std::runtime_error("Content size exceeds maximum allowed ("
                            + std::to_string(receivedTotal / 1024) + "KB > "
                            + std::to_string(maxBodySize / 1024) + "KB)");
                    }

                    if (leftovers.size() - receivedTotal < 256) {
                        cursor = 0;
                        m_file << leftovers.substr(0, receivedTotal);
                    }

                    buffer = leftovers.data() + cursor;
                    len = leftovers.size() - cursor;
                    return true;
                }
            );
        }
        catch (const std::exception& e)
        {
            m_file.clear();
            throw std::runtime_error(std::string("Body parse error: ") + e.what());
        }
        return initSize;
    }

    size_t StringBody::sendTransferSize(Socket& sock, size_t size,
        size_t maxRetryCount, size_t maxBodySize)
    {
        try
        {
            if (maxBodySize < m_size)
                throw std::runtime_error("Body size exceeds maximum allowed size");

            return sock.sendCommited(m_data.data(), m_data.size(), maxRetryCount);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("Body send error: ") + e.what());
        }
    }

    size_t StringBody::sendChunked(Socket& sock, size_t maxRetryCount,
        size_t maxBodySize)
    {
        return 0;

    }

    size_t FileBody::sendTransferSize(Socket& sock, size_t size,
        size_t maxRetryCount, size_t maxBodySize)
    {
        std::cout << "Sending body transfer size: " << std::endl;

        m_file.seekg(0, std::ios::beg);
        if (m_size == 0)
            throw std::runtime_error("Body has no content to send");
        try
        {
            if (maxBodySize < m_size)
                throw std::runtime_error("Body size exceeds maximum allowed size");

            std::vector<char> buffer(1024);
            size_t bytesRead = m_file.read(buffer.data(), buffer.size()).gcount();

            if (bytesRead < buffer.size()) {
                // Small file case - single send
                std::cout << "Sending the message in one go" << std::endl;
                return sock.sendCommited(buffer.data(), bytesRead, maxRetryCount);
            }

            std::cout << "Sending the message iteratively" << std::endl;

            return sock.sendLoop(buffer.data(), buffer.size(), 0, maxRetryCount,
                [this, &buffer](char*& buf, size_t& len, size_t bytesSent, size_t& receivedTotal) {
                    for (size_t i = bytesSent; i < len; i++)
                        buffer[i - bytesSent] = buffer[i];

                    size_t bytesLeft = len - bytesSent;
                    size_t bytesRead = m_file.read(buffer.data() + bytesLeft, buffer.size() - bytesLeft).gcount();

                    len -= bytesSent - bytesRead;
                    if (len > 0)
                        return true;
                    return false;
                });
            
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("Body send error: ") + e.what());
        }
    }

    size_t FileBody::sendChunked(Socket& sock, size_t maxRetryCount,
        size_t maxBodySize)
    {
        m_file.seekg(0, std::ios::beg);
        return 0;
    }
}