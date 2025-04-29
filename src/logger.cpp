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

#include "logger.h"

#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

// Initialize static members
std::unique_ptr<Logger> Logger::s_instance = nullptr;
std::once_flag Logger::s_onceFlag;

Logger::Logger()
    : m_initialized(false),
      m_level(Level::INFO),
      m_nextCallbackId(1)
{
}

Logger::~Logger()
{
    close();
}

Logger* Logger::GetInstance()
{
    std::call_once(s_onceFlag, []() {
        s_instance = std::unique_ptr<Logger>(new Logger());
    });
    
    return s_instance.get();
}

bool Logger::initialize(const std::string& logFile)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close existing log file
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    // Open new log file
    m_logFile.open(logFile, std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        return false;
    }
    
    m_logFileName = logFile;
    m_initialized = true;
    
    // Log initialization message
    LogMessage message;
    message.timestamp = std::chrono::system_clock::now();
    message.level = Level::INFO;
    message.message = "Logger initialized";
    message.file = __FILE__;
    message.line = __LINE__;
    
    writeToFile(message);
    
    return true;
}

void Logger::close()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Log closing message
    if (m_initialized) {
        LogMessage message;
        message.timestamp = std::chrono::system_clock::now();
        message.level = Level::INFO;
        message.message = "Logger closed";
        message.file = __FILE__;
        message.line = __LINE__;
        
        writeToFile(message);
    }
    
    // Close log file
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    m_initialized = false;
}

void Logger::setLevel(Level level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

Logger::Level Logger::getLevel() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

int Logger::registerCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!callback) {
        return 0;
    }
    
    // Create callback info
    CallbackInfo info;
    info.id = m_nextCallbackId++;
    info.callback = callback;
    
    // Add to callbacks vector
    m_callbacks.push_back(info);
    
    return info.id;
}

void Logger::unregisterCallback(int callbackId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find and remove callback
    for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
        if (it->id == callbackId) {
            m_callbacks.erase(it);
            break;
        }
    }
}

void Logger::debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logv(Level::DEBUG, format, args);
    va_end(args);
}

void Logger::info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logv(Level::INFO, format, args);
    va_end(args);
}

void Logger::warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logv(Level::WARN, format, args);
    va_end(args);
}

void Logger::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logv(Level::ERROR, format, args);
    va_end(args);
}

void Logger::fatal(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logv(Level::FATAL, format, args);
    va_end(args);
}

void Logger::log(Level level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logv(level, format, args);
    va_end(args);
}

void Logger::logv(Level level, const char* format, va_list args)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check log level
    if (level < m_level) {
        return;
    }
    
    // Format message
    char buffer[FORMAT_BUFFER_SIZE];
    vsnprintf(buffer, FORMAT_BUFFER_SIZE, format, args);
    
    // Create log message
    LogMessage message;
    message.timestamp = std::chrono::system_clock::now();
    message.level = level;
    message.message = buffer;
    message.file = "";
    message.line = 0;
    
    // Write to file
    if (m_initialized) {
        writeToFile(message);
    }
    
    // Notify callbacks
    notifyCallbacks(message);
}

const char* Logger::getLevelString(Level level)
{
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        case Level::FATAL: return "FATAL";
        default:           return "UNKNOWN";
    }
}

void Logger::writeToFile(const LogMessage& message)
{
    // Check if file is open
    if (!m_logFile.is_open()) {
        return;
    }
    
    // Format timestamp
    auto timeT = std::chrono::system_clock::to_time_t(message.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        message.timestamp.time_since_epoch() % std::chrono::seconds(1)).count();
    
    std::tm tm;
    
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    
    // Write to log file
    m_logFile << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." 
             << std::setfill('0') << std::setw(3) << ms << " "
             << "[" << getLevelString(message.level) << "] "
             << message.message;
    
    // Add file and line if available
    if (!message.file.empty()) {
        m_logFile << " (" << message.file << ":" << message.line << ")";
    }
    
    m_logFile << std::endl;
    m_logFile.flush();
}

void Logger::notifyCallbacks(const LogMessage& message)
{
    // Call all registered callbacks
    for (const auto& callbackInfo : m_callbacks) {
        try {
            callbackInfo.callback(message);
        } catch (...) {
            // Ignore callback exceptions
        }
    }
}