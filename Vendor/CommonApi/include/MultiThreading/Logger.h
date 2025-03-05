#pragma once
#include "Synchronized.h"
#include "FileSystem.h"

#include <array>
#include <iostream>

namespace MultiThreading
{
    //all logging is enabled by default
    class Logger {
    public:
        // Different logging levels
        enum class Level {
            TRACE = 0,     // Most detailed level, for tracing code execution
            DEBUG,         // Debugging information, variables, state
            INFO,          // General information about program execution
            NOTICE,        // Normal but significant conditions
            WARNING,       // Warning messages, potential issues
            ERROR,         // Error messages, recoverable failures
            CRITICAL,      // Critical errors that may prevent normal operation
            ALERT,         // Action must be taken immediately
            EMERGENCY,     // System is unusable
            OFF,           // No logging
        };

        static inline const std::array<std::string, 10> LevelNames = {
            "TRACE",
            "DEBUG",
            "INFO",
            "NOTICE",
            "WARNING",
            "ERROR",
            "CRITICAL",
            "ALERT",
            "EMERGENCY"
            "OFF"
        };

        struct LogMessage {
            Level level;
            std::string message;
            std::chrono::system_clock::time_point timestamp;
        };

    private:

        class MessageBuffer {
        private:
            std::vector<LogMessage> buffer;
            size_t maxSize;
            size_t currentIndex = 0;

        public:
            explicit MessageBuffer(size_t size = 1000) : maxSize(size) {
                buffer.reserve(maxSize);
            }

            void resize(size_t size){
                maxSize = size;
                buffer.clear();
            }

            bool add(LogMessage msg) {
                if (buffer.size() < maxSize) {
                    buffer.push_back(std::move(msg));
                    if (buffer.size() >= maxSize)
                        return 1;
                    return 0;
                }
                else {
                    // Circular buffer behavior
                    buffer[currentIndex] = std::move(msg);
                    currentIndex = (currentIndex + 1) % maxSize;
                }
            }

            // Get all messages for writing to file
            std::vector<LogMessage>& getLog() {
                return buffer;
            }

            // Clear the buffer after writing to file
            void clear() {
                buffer.clear();
                currentIndex = 0;
            }
        };

        std::mutex m_consoleMutex; //temporary until i have a thread safe output system
        std::shared_mutex m_mutex;

        bool m_consoleOutputEnabled = 1;
        bool m_fileOutputEnabled = 0;
        bool m_shouldLog = 1;
        Level m_minimumLogLevel = Level::TRACE;
        Level m_minimumConsoleOutputLevel = Level::TRACE;

        std::string m_logFilePath;
        std::string m_logFileName;
        std::string m_logFileFormat;
        std::string m_fullFilePath;

        Synchronized<MessageBuffer> buffer;

    public:

        class LogStream {
        public:
            LogStream(Logger& logger, Level level)
                : m_logger(logger), m_level(level) {}

            template<typename T>
            LogStream& operator<<(const T& data) {
                m_buffer << data;
                return *this;
            }

            LogStream(const LogStream& other) = delete;
            LogStream& operator=(const LogStream& other) = delete;
            LogStream(LogStream&& other) = delete;
            LogStream& operator=(LogStream&& other) = delete;

            LogStream& operator<<(LogStream& (*manip)(LogStream&)) {
                manip(*this);
                return *this;
            }

            ~LogStream() {
                m_logger.log(m_level, m_buffer.str());
            }

            inline LogStream& endLog(LogStream& os) {
                m_logger.log(m_level, m_buffer.str());
                m_buffer.clear();
                return os;
            }

        private:
            Logger& m_logger;
            Level m_level;
            std::ostringstream m_buffer;

        };

        std::ostream& getStreamForLevel(Level level) {
            switch (level) {
                // Normal information goes to cout
            case Level::TRACE:
            case Level::DEBUG:
            case Level::INFO:
            case Level::NOTICE:
                return std::cout;

                // Warnings and errors go to cerr for immediate output
            case Level::WARNING:
            case Level::ERROR:
            case Level::CRITICAL:
            case Level::ALERT:
            case Level::EMERGENCY:
                return std::cerr;

                // Diagnostic/verbose logging goes to clog
            case Level::OFF:
            default:
                return std::clog;
            }
        }

        // Convenience methods for all levels
        LogStream trace()       { return LogStream(*this, Level::TRACE);        }
        LogStream debug()       { return LogStream(*this, Level::DEBUG);        }
        LogStream info()        { return LogStream(*this, Level::INFO);         }
        LogStream notice()      { return LogStream(*this, Level::NOTICE);       }
        LogStream warning()     { return LogStream(*this, Level::WARNING);      }
        LogStream error()       { return LogStream(*this, Level::ERROR);        }
        LogStream critical()    { return LogStream(*this, Level::CRITICAL);     }
        LogStream alert()       { return LogStream(*this, Level::ALERT);        }
        LogStream emergency()   { return LogStream(*this, Level::EMERGENCY);    }

        void log(Level level, const std::string& message);

        // Helper to convert level to string
        static std::string getLevelString(Level level) {
            return LevelNames[static_cast<int>(level)];
        }

        void flush();

        // Configuration functions
        void setLogLevel(Level level) { 
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            m_minimumLogLevel = level; 
        };
        void setConsoleLogLevel(Level level) { 
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            m_minimumConsoleOutputLevel = level; 
        };
        void setLogFile(const std::string& path,
            const std::string& filename, const std::string& format);

        //Warning! clears the buffer
        void setBufferSize(size_t size) {
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            buffer.getWriteAccess()->resize(size);
        };

        // Toggle features
        void toggleConsoleOutput(bool enable) {
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            m_consoleOutputEnabled = enable;
        };

        //do not use before specifying default folder
        void toggleFileOutput(bool enable) {
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            m_fileOutputEnabled = enable;
        };
        
    private:
        //for automatic flushing
        void flush(MultiThreading::Synchronized<MessageBuffer>::WriteAccess& access);

        // Helper functions
        std::string formatMessage(const LogMessage& message);
        std::string formatTimestamp(const LogMessage& message);
        void writeToFile(const std::string& message);
        void writeToConsole(const std::string& message);
        bool shouldLog(Level m_level) {
            return m_shouldLog && m_minimumLogLevel <= m_level;
        };
    };
}

