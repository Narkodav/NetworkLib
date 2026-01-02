#pragma once
#include <cstddef>
#include <string_view>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

template <bool writable = false>
class MappedFile {
public:

private:
    using DataPointer = std::conditional_t<writable, char*, const char*>;
    
    DataPointer m_data = nullptr;
    size_t m_size = 0;
#ifdef _WIN32
    void* m_file_handle = nullptr;
    void* m_mapping_handle = nullptr;
#else
    int m_fd = -1;
#endif

public:
    
    MappedFile() = default;
    MappedFile(const char* filename)
    {
        map(filename);
    }

    ~MappedFile()
    {
        unmap();
    }

    MappedFile(MappedFile&& other) noexcept
        : m_data(std::exchange(other.m_data, nullptr)),
        m_size(std::exchange(other.m_size, 0)),
#ifdef _WIN32
        m_file_handle(std::exchange(other.m_file_handle, nullptr)),
        m_mapping_handle(std::exchange(other.m_mapping_handle, nullptr))
#else
        m_fd(std::exchange(other.m_fd, -1))
#endif
    {
    }

    MappedFile& operator=(MappedFile&& other) noexcept
    {
        if (this == &other)
            return *this;
        unmap();
        m_data = std::exchange(other.m_data, nullptr);
        m_size = std::exchange(other.m_size, 0);
#ifdef _WIN32
        m_file_handle = std::exchange(other.m_file_handle, nullptr);
        m_mapping_handle = std::exchange(other.m_mapping_handle, nullptr);
#else
        m_fd = std::exchange(other.m_fd, -1);
#endif
        return *this;
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    inline size_t size() const noexcept { return m_size; }
    inline bool empty() const noexcept { return m_size == 0; }

    inline const char* data() const noexcept { return m_data; }
    inline const char& operator[](size_t i) const { return m_data[i]; };
    inline const char& at(size_t i) const {
        if (i >= m_size) throw std::out_of_range("MappedFile::at");
        return m_data[i];
    }

    inline char* data() noexcept requires (writable == true) { return m_data; }
    inline char& operator[](size_t i) noexcept requires (writable == true) { return m_data[i]; };
    inline char& at(size_t i) requires (writable == true) {
        return const_cast<char&>(const_cast<const MappedFile&>(*this).at(i));
    }

    // std::string_view compatibility
    operator std::string_view() const { return { m_data, m_size }; }

    void map(const char* filename)
    {
#ifdef _WIN32
        // Windows implementation

        if constexpr (writable)
            m_file_handle = CreateFileA(
                filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
            );
        else         m_file_handle = CreateFileA(
            filename, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
        );

        if (m_file_handle == INVALID_HANDLE_VALUE) {
            throw std::system_error(GetLastError(), std::system_category());
        }

        LARGE_INTEGER file_size;
        GetFileSizeEx(m_file_handle, &file_size);
        m_size = static_cast<std::size_t>(file_size.QuadPart);

        if constexpr (writable)
            m_mapping_handle = CreateFileMappingA(
                m_file_handle, NULL, PAGE_READWRITE, 0, 0, NULL
            );
        else m_mapping_handle = CreateFileMappingA(
            m_file_handle, NULL, PAGE_READONLY, 0, 0, NULL
        );


        if (!m_mapping_handle) {
            CloseHandle(m_file_handle);
            throw std::system_error(GetLastError(), std::system_category());
        }

        if constexpr (writable)
            m_data = static_cast<const char*>(MapViewOfFile(
                m_mapping_handle, FILE_MAP_WRITE, 0, 0, m_size
            ));
        else m_data = static_cast<const char*>(MapViewOfFile(
            m_mapping_handle, FILE_MAP_READ, 0, 0, m_size
        ));

        if (!m_data) {
            CloseHandle(m_mapping_handle);
            CloseHandle(m_file_handle);
            throw std::system_error(GetLastError(), std::system_category());
        }
#else
        // Unix-like (Linux, macOS)
        m_fd = open(filename, O_RDONLY);
        if (m_fd == -1) {
            throw std::system_error(errno, std::generic_category());
        }

        struct stat sb;
        if (fstat(m_fd, &sb) == -1) {
            close(m_fd);
            throw std::system_error(errno, std::generic_category());
        }
        m_size = sb.st_size;

        if constexpr (writable) {
            m_data = static_cast<char*>(mmap(
                nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0
            ));
        }
        else m_data = static_cast<const char*>(mmap(
            nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0
        ));

        if (m_data == MAP_FAILED) {
            close(m_fd);
            throw std::system_error(errno, std::generic_category());
        }
#endif
    }
    void unmap()
    {
        if (m_data) {
#ifdef _WIN32
            UnmapViewOfFile(m_data);
            CloseHandle(m_mapping_handle);
            CloseHandle(m_file_handle);
            m_file_handle = nullptr;
            m_mapping_handle = nullptr;
#else
            munmap(const_cast<char*>(m_data), m_size);
            close(m_fd);
            m_fd = -1;
#endif
            m_data = nullptr;
            m_size = 0;
        }
    }

    bool isMapped() const noexcept { return m_data != nullptr; };
};