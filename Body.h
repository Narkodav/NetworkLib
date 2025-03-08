#pragma once
#include "Common.h"
#include "Socket.h"

namespace http {

    class Body {
    public:
        enum class Type
        {
            STRING,
            FILE,
        };

    protected:
        size_t m_size = 0;
    public:
        virtual ~Body() = default;

        // Common access patterns
        size_t size() const { return m_size; };
        bool empty() const { return m_size; };

        // Read interface
        virtual size_t read(char* dest, size_t offset, size_t length) const = 0;

        // Write interface
        virtual void write(const char* data, size_t length) = 0;
        virtual void append(const char* data, size_t length) = 0;

        virtual Type getType() const = 0;

        virtual size_t readTransferSize(Socket& sock, std::string& leftovers,
            size_t size, size_t maxRetryCount, size_t maxBodySize) = 0;
        virtual size_t readChunked(Socket& sock, std::string& leftovers,
            size_t maxRetryCount, size_t maxBodySize) = 0;

        virtual size_t sendTransferSize(Socket& sock, size_t size,
            size_t maxRetryCount, size_t maxBodySize) = 0;
        virtual size_t sendChunked(Socket& sock, size_t maxRetryCount,
            size_t maxBodySize) = 0;
    };

    // In-memory implementation
    class StringBody : public Body {
    private:
        std::string m_data;

    public:

        size_t read(char* dest, size_t offset, size_t length) const override {
            if (offset >= m_data.size()) return 0;
            size_t available = min(length, m_data.size() - offset);
            std::memcpy(dest, m_data.data() + offset, available);
            return available;
        }

        void write(const char* data, size_t length) override {
            m_data.assign(data, length);
        }

        void append(const char* data, size_t length) override {
            m_data.append(data, length);
        }

        Type getType() const override { return Type::STRING; };

        const char* data() const { return m_data.data(); }

        virtual size_t readTransferSize(Socket& sock, std::string& leftovers,
            size_t size, size_t maxRetryCount, size_t maxBodySize) override;
        virtual size_t readChunked(Socket& sock, std::string& leftovers,
            size_t maxRetryCount, size_t maxBodySize) override;

        virtual size_t sendTransferSize(Socket& sock, size_t size,
            size_t maxRetryCount, size_t maxBodySize) override;
        virtual size_t sendChunked(Socket& sock, size_t maxRetryCount,
            size_t maxBodySize) override;
    };

    // File-based implementation
    class FileBody : public Body {
    private:
        mutable std::fstream m_file;
        std::string m_path;

    public:
        explicit FileBody(const std::string& path)
            : m_path(path) {
            m_file.open(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
            if (!m_file) throw std::runtime_error("Cannot open file buffer");
            m_size = std::filesystem::file_size(path);
        }

        size_t read(char* dest, size_t offset, size_t length) const override {
            if (offset >= m_size) return 0;
            m_file.seekg(offset);
            m_file.read(dest, length);
            return m_file.gcount();
        }

        void write(const char* data, size_t length) override {
            m_file.seekp(0);
            m_file.write(data, length);
            m_size = length;
        }

        void append(const char* data, size_t length) override {
            m_file.seekp(m_size);
            m_file.write(data, length);
            m_size += length;
        }

        Type getType() const override { return Type::FILE; };

        virtual size_t readTransferSize(Socket& sock, std::string& leftovers,
            size_t size, size_t maxRetryCount, size_t maxBodySize) override;
        virtual size_t readChunked(Socket& sock, std::string& leftovers,
            size_t maxRetryCount, size_t maxBodySize) override;

        virtual size_t sendTransferSize(Socket& sock, size_t size,
            size_t maxRetryCount, size_t maxBodySize) override;
        virtual size_t sendChunked(Socket& sock, size_t maxRetryCount,
            size_t maxBodySize) override;
    };

    //class BodyFactory
    //{
    //public:
    //    static std::unique_ptr<Body> getBody(Body::Type type) {
    //        switch (type) {
    //        case Body::Type::STRING:
    //            return std::make_unique<StringBody>();
    //        case Body::Type::FILE:
    //            return std::make_unique<FileBody>();
    //        default:
    //            throw std::runtime_error("Unknown body type");
    //        }
    //    }
    //};
}