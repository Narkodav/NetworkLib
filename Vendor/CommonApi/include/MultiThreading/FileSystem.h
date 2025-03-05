#pragma once
#include "Synchronized.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <string>
#include <functional>

namespace MultiThreading
{
    namespace  fs = std::filesystem;

    class FileSystem
    {
        using FileMutexMap = Synchronized<std::unordered_map<std::string,
            std::shared_ptr<std::shared_mutex>>>;

        static inline FileMutexMap fileMutexes;
        static inline std::shared_mutex directoryMutex;
    public:
        enum class CreateMode {
            Create,      // Create new file, fail if exists
            Truncate,    // Create or overwrite
            Append,      // Append to existing file, fail if doesnt exist
            AppendNew    // Create if doesn't exist, append if exists
        };

        static bool exists(const std::string& path);
        static bool isDirectory(const std::string& path);
        static std::vector<std::string> listDirectory(const std::string& path);
        static bool createDirectory(const std::string& path);
        static bool createDirectories(const std::string& path);
        static bool removeFile(const std::string& path);

        static void readFileByLine(const std::string& path, std::function<void(const std::string&)> lineProcessor);
        static void readFileBinaryBatched(const std::string& path,
            std::function<void(const char* data, size_t size)> batchProcessor,
            size_t batchSize = 8192); //batch size in bytes, default 8kb

        static std::vector<char> readFileBinaryRange(const std::string& path,
            size_t offset,
            size_t length);

        static std::string readFileText(const std::string& path);
        static std::vector<char> readFileBinary(const std::string& path);

        static bool writeFileText(const std::string& path,
            const std::string& content,
            CreateMode mode = CreateMode::Truncate);

        static bool writeFileBinary(const std::string& path,
            const std::vector<char>& content,
            CreateMode mode = CreateMode::Truncate);

    private:
        static std::shared_ptr<std::shared_mutex> getFileMutex(const std::string& path);

    };
}
