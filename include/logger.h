/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X86EMULATOR_LOGGER_H
#define X86EMULATOR_LOGGER_H

#include <string>
#include <mutex>
#include <fstream>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>

/**
 * @brief Logger for the emulator
 * 
 * Provides logging functionality with different log levels
 */
class Logger {
public:
    /**
     * @brief Log levels
     */
    enum class Level {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };
    
    /**
     * @brief Log message structure
     */
    struct LogMessage {
        std::chrono::system_clock::time_point timestamp;
        Level level;
        std::string message;
        std::string file;
        int line;
    };
    
    /**
     * @brief Log callback function type
     */
    using LogCallback = std::function<void(const LogMessage&)>;
    
    /**
     * @brief Get the singleton instance of the logger
     * 
     * @return Pointer to the logger instance
     */
    static Logger* GetInstance();
    
    /**
     * @brief Initialize the logger with a log file
     * 
     * @param logFile Path to the log file
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize(const std::string& logFile);
    
    /**
     * @brief Close the logger
     */
    void close();
    
    /**
     * @brief Set the log level
     * 
     * @param level Minimum log level to output
     */
    void setLevel(Level level);
    
    /**
     * @brief Get the current log level
     * 
     * @return Current log level
     */
    Level getLevel() const;
    
    /**
     * @brief Register a log callback
     * 
     * @param callback Callback function
     * @return Callback ID
     */
    int registerCallback(LogCallback callback);
    
    /**
     * @brief Unregister a log callback
     * 
     * @param callbackId Callback ID
     */
    void unregisterCallback(int callbackId);
    
    /**
     * @brief Log a debug message
     * 
     * @param format Format string
     * @param ... Format arguments
     */
    void debug(const char* format, ...);
    
    /**
     * @brief Log an info message
     * 
     * @param format Format string
     * @param ... Format arguments
     */
    void info(const char* format, ...);
    
    /**
     * @brief Log a warning message
     * 
     * @param format Format string
     * @param ... Format arguments
     */
    void warn(const char* format, ...);
    
    /**
     * @brief Log an error message
     * 
     * @param format Format string
     * @param ... Format arguments
     */
    void error(const char* format, ...);
    
    /**
     * @brief Log a fatal message
     * 
     * @param format Format string
     * @param ... Format arguments
     */
    void fatal(const char* format, ...);
    
    /**
     * @brief Log a message with a specific level
     * 
     * @param level Log level
     * @param format Format string
     * @param ... Format arguments
     */
    void log(Level level, const char* format, ...);
    
    /**
     * @brief Log a message with a specific level from a va_list
     * 
     * @param level Log level
     * @param format Format string
     * @param args Variable argument list
     */
    void logv(Level level, const char* format, va_list args);
    
    /**
     * @brief Get the string representation of a log level
     * 
     * @param level Log level
     * @return String representation
     */
    static const char* getLevelString(Level level);

private:
    /**
     * @brief Construct a new Logger
     */
    Logger();
    
    /**
     * @brief Destroy the Logger
     */
    ~Logger();
    
    // Prevent copying and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Internal methods
    void writeToFile(const LogMessage& message);
    void notifyCallbacks(const LogMessage& message);
    
    // Log file
    std::ofstream m_logFile;
    std::string m_logFileName;
    bool m_initialized;
    
    // Log level
    Level m_level;
    
    // Callbacks
    struct CallbackInfo {
        int id;
        LogCallback callback;
    };
    
    std::vector<CallbackInfo> m_callbacks;
    int m_nextCallbackId;
    
    // Format buffer size
    static constexpr size_t FORMAT_BUFFER_SIZE = 4096;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;
    
    // Singleton instance
    static std::unique_ptr<Logger> s_instance;
    static std::once_flag s_onceFlag;
};

#endif // X86EMULATOR_LOGGER_H