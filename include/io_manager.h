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

#ifndef X86EMULATOR_IO_MANAGER_H
#define X86EMULATOR_IO_MANAGER_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <mutex>

/**
 * @brief I/O port manager for the emulator
 * 
 * Handles I/O port access and device registration.
 */
class IOManager {
public:
    /**
     * @brief Callback type for I/O port read
     */
    using IOReadCallback = std::function<uint8_t(uint16_t)>;
    
    /**
     * @brief Callback type for I/O port write
     */
    using IOWriteCallback = std::function<void(uint16_t, uint8_t)>;
    
    /**
     * @brief I/O port range descriptor
     */
    struct IOPortRange {
        uint16_t start;      // Start port
        uint16_t end;        // End port (inclusive)
        std::string device;  // Device name
        IOReadCallback readCallback;  // Read callback
        IOWriteCallback writeCallback;  // Write callback
    };
    
    /**
     * @brief Construct a new IOManager
     */
    IOManager();
    
    /**
     * @brief Destroy the IOManager
     */
    ~IOManager();
    
    /**
     * @brief Initialize I/O subsystem
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Read a byte from an I/O port
     * 
     * @param port I/O port
     * @return Byte value
     */
    uint8_t readByte(uint16_t port) const;
    
    /**
     * @brief Read a word from an I/O port
     * 
     * @param port I/O port
     * @return Word value
     */
    uint16_t readWord(uint16_t port) const;
    
    /**
     * @brief Read a double word from an I/O port
     * 
     * @param port I/O port
     * @return Double word value
     */
    uint32_t readDword(uint16_t port) const;
    
    /**
     * @brief Write a byte to an I/O port
     * 
     * @param port I/O port
     * @param value Byte value
     */
    void writeByte(uint16_t port, uint8_t value) const;
    
    /**
     * @brief Write a word to an I/O port
     * 
     * @param port I/O port
     * @param value Word value
     */
    void writeWord(uint16_t port, uint16_t value) const;
    
    /**
     * @brief Write a double word to an I/O port
     * 
     * @param port I/O port
     * @param value Double word value
     */
    void writeDword(uint16_t port, uint32_t value) const;
    
    /**
     * @brief Register I/O port range
     * 
     * @param startPort Start port
     * @param endPort End port (inclusive)
     * @param device Device name
     * @param readCallback Callback for port reads
     * @param writeCallback Callback for port writes
     * @return true if registration was successful
     * @return false if registration failed
     */
    bool registerIOPortRange(uint16_t startPort, uint16_t endPort, const std::string& device,
                             IOReadCallback readCallback, IOWriteCallback writeCallback);
    
    /**
     * @brief Unregister I/O port range
     * 
     * @param startPort Start port
     * @param endPort End port (inclusive)
     * @return true if unregistration was successful
     * @return false if unregistration failed
     */
    bool unregisterIOPortRange(uint16_t startPort, uint16_t endPort);
    
    /**
     * @brief Get I/O port range information
     * 
     * @param port Port number
     * @return Pointer to port range info, or nullptr if not registered
     */
    const IOPortRange* getIOPortRange(uint16_t port) const;
    
    /**
     * @brief Get all registered I/O port ranges
     * 
     * @return Vector of registered port ranges
     */
    std::vector<IOPortRange> getIOPortRanges() const;
    
    /**
     * @brief Reset I/O subsystem
     */
    void reset();

private:
    std::vector<IOPortRange> m_portRanges;
    mutable std::mutex m_mutex;
    
    // Default handlers for unregistered ports
    uint8_t defaultIOReadByte(uint16_t port) const;
    void defaultIOWriteByte(uint16_t port, uint8_t value) const;
};

#endif // X86EMULATOR_IO_MANAGER_H