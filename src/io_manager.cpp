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

#include "io_manager.h"
#include "logger.h"

IOManager::IOManager()
{
}

IOManager::~IOManager()
{
    // Clear port ranges
    m_portRanges.clear();
}

bool IOManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Clear existing port ranges
        m_portRanges.clear();
        
        Logger::GetInstance()->info("I/O subsystem initialized");
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("I/O initialization failed: %s", ex.what());
        return false;
    }
}

uint8_t IOManager::readByte(uint16_t port) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find port handler
    const IOPortRange* range = getIOPortRange(port);
    if (range && range->readCallback) {
        return range->readCallback(port);
    }
    
    // Use default handler
    return defaultIOReadByte(port);
}

uint16_t IOManager::readWord(uint16_t port) const
{
    // Read two bytes
    uint8_t low = readByte(port);
    uint8_t high = readByte(port + 1);
    
    // Combine into word (little-endian)
    return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

uint32_t IOManager::readDword(uint16_t port) const
{
    // Read two words
    uint16_t low = readWord(port);
    uint16_t high = readWord(port + 2);
    
    // Combine into dword (little-endian)
    return static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 16);
}

void IOManager::writeByte(uint16_t port, uint8_t value) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find port handler
    const IOPortRange* range = getIOPortRange(port);
    if (range && range->writeCallback) {
        range->writeCallback(port, value);
        return;
    }
    
    // Use default handler
    defaultIOWriteByte(port, value);
}

void IOManager::writeWord(uint16_t port, uint16_t value) const
{
    // Write two bytes (little-endian)
    writeByte(port, static_cast<uint8_t>(value));
    writeByte(port + 1, static_cast<uint8_t>(value >> 8));
}

void IOManager::writeDword(uint16_t port, uint32_t value) const
{
    // Write two words (little-endian)
    writeWord(port, static_cast<uint16_t>(value));
    writeWord(port + 2, static_cast<uint16_t>(value >> 16));
}

bool IOManager::registerIOPortRange(uint16_t startPort, uint16_t endPort, const std::string& device,
                                    IOReadCallback readCallback, IOWriteCallback writeCallback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check for invalid range
    if (startPort > endPort) {
        Logger::GetInstance()->error("Invalid I/O port range: %04X-%04X", startPort, endPort);
        return false;
    }
    
    // Check for overlapping ranges
    for (const auto& range : m_portRanges) {
        if ((startPort <= range.end) && (endPort >= range.start)) {
            Logger::GetInstance()->error("I/O port range %04X-%04X overlaps with existing range %04X-%04X (%s)",
                                      startPort, endPort, range.start, range.end, range.device.c_str());
            return false;
        }
    }
    
    // Create new range
    IOPortRange range;
    range.start = startPort;
    range.end = endPort;
    range.device = device;
    range.readCallback = readCallback;
    range.writeCallback = writeCallback;
    
    // Add to ranges vector
    m_portRanges.push_back(range);
    
    Logger::GetInstance()->info("Registered I/O port range %04X-%04X for %s", startPort, endPort, device.c_str());
    return true;
}

bool IOManager::unregisterIOPortRange(uint16_t startPort, uint16_t endPort)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find range
    for (auto it = m_portRanges.begin(); it != m_portRanges.end(); ++it) {
        if (it->start == startPort && it->end == endPort) {
            // Remove range
            std::string device = it->device;
            m_portRanges.erase(it);
            
            Logger::GetInstance()->info("Unregistered I/O port range %04X-%04X for %s", startPort, endPort, device.c_str());
            return true;
        }
    }
    
    Logger::GetInstance()->warn("I/O port range %04X-%04X not found", startPort, endPort);
    return false;
}

const IOManager::IOPortRange* IOManager::getIOPortRange(uint16_t port) const
{
    // Find range containing port
    for (const auto& range : m_portRanges) {
        if (port >= range.start && port <= range.end) {
            return &range;
        }
    }
    
    return nullptr;
}

std::vector<IOManager::IOPortRange> IOManager::getIOPortRanges() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_portRanges;
}

void IOManager::reset()
{
    // No need to reset anything for now, but might need to in the future
    // if we start keeping state in this class
}

uint8_t IOManager::defaultIOReadByte(uint16_t port) const
{
    // Default handler returns 0xFF for unregistered ports
    Logger::GetInstance()->debug("I/O read from unhandled port: 0x%04X", port);
    return 0xFF;
}

void IOManager::defaultIOWriteByte(uint16_t port, uint8_t value) const
{
    // Default handler ignores writes to unregistered ports
    Logger::GetInstance()->debug("I/O write to unhandled port: 0x%04X value 0x%02X", port, value);
}