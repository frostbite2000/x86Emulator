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

#ifndef X86EMULATOR_NETWORK_ADAPTER_H
#define X86EMULATOR_NETWORK_ADAPTER_H

#include "device.h"
#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <array>

/**
 * @brief Network adapter emulation
 */
class NetworkAdapter : public Device {
public:
    /**
     * @brief Network adapter types
     */
    enum class AdapterType {
        NE2000,     ///< NE2000 compatible
        RTL8139,    ///< Realtek RTL8139
        PCAP        ///< PCap/host bridge
    };
    
    /**
     * @brief Network packet structure
     */
    struct Packet {
        std::vector<uint8_t> data;  ///< Packet data
        uint32_t size;              ///< Packet size
        bool broadcast;             ///< Is broadcast packet
        bool multicast;             ///< Is multicast packet
    };
    
    /**
     * @brief Construct a new NetworkAdapter
     * 
     * @param cardType Card type (e.g., "ne2000", "rtl8139")
     */
    explicit NetworkAdapter(const std::string& cardType);
    
    /**
     * @brief Destroy the NetworkAdapter
     */
    ~NetworkAdapter() override;
    
    /**
     * @brief Initialize the network adapter
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the network adapter
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the network adapter
     */
    void reset() override;
    
    /**
     * @brief Read from a network register
     * 
     * @param port Register port
     * @return Register value
     */
    uint8_t readRegister(uint16_t port) const;
    
    /**
     * @brief Write to a network register
     * 
     * @param port Register port
     * @param value Value to write
     */
    void writeRegister(uint16_t port, uint8_t value);
    
    /**
     * @brief Read from network memory
     * 
     * @param address Memory address
     * @return Memory value
     */
    uint8_t readMemory(uint32_t address) const;
    
    /**
     * @brief Write to network memory
     * 
     * @param address Memory address
     * @param value Value to write
     */
    void writeMemory(uint32_t address, uint8_t value);
    
    /**
     * @brief Send a packet
     * 
     * @param data Packet data
     * @param size Packet size
     * @return true if packet was sent successfully
     * @return false if packet could not be sent
     */
    bool sendPacket(const uint8_t* data, uint32_t size);
    
    /**
     * @brief Receive a packet
     * 
     * @param data Buffer to store packet data
     * @param maxSize Maximum size of the buffer
     * @return Size of received packet, or 0 if no packet was received
     */
    uint32_t receivePacket(uint8_t* data, uint32_t maxSize);
    
    /**
     * @brief Check if the adapter is active
     * 
     * @return true if the adapter is active
     * @return false if the adapter is not active
     */
    bool isActive() const override;
    
    /**
     * @brief Get MAC address
     * 
     * @return MAC address as array of 6 bytes
     */
    const std::array<uint8_t, 6>& getMACAddress() const;
    
    /**
     * @brief Set MAC address
     * 
     * @param mac MAC address as array of 6 bytes
     */
    void setMACAddress(const std::array<uint8_t, 6>& mac);
    
    /**
     * @brief Get the adapter type
     * 
     * @return Adapter type
     */
    AdapterType getAdapterType() const;
    
    /**
     * @brief Set packet receive callback
     * 
     * @param callback Callback function
     */
    void setReceiveCallback(std::function<void(const uint8_t*, uint32_t)> callback);

private:
    // Network adapter properties
    AdapterType m_adapterType;
    std::string m_cardType;
    std::array<uint8_t, 6> m_macAddress;
    
    // Packet queues
    std::queue<Packet> m_receiveQueue;
    std::queue<Packet> m_transmitQueue;
    
    // Network adapter state
    bool m_enabled;
    bool m_promiscuousMode;
    bool m_multicastMode;
    
    // Activity state and timeout
    bool m_transmitActive;
    bool m_receiveActive;
    int m_activityTimeout;
    
    // I/O ports
    uint16_t m_baseIOPort;
    
    // Memory
    std::vector<uint8_t> m_memory;
    uint16_t m_memorySize;
    
    // Registers
    union {
        struct {
            uint8_t cr;      // Command Register
            uint8_t isr;     // Interrupt Status Register
            uint8_t imr;     // Interrupt Mask Register
            uint8_t rcr;     // Receive Configuration Register
            uint8_t tcr;     // Transmit Configuration Register
            uint8_t dcr;     // Data Configuration Register
            uint8_t rsr;     // Receive Status Register
            uint8_t page0;   // Page 0 Register
            uint8_t page1;   // Page 1 Register
            uint8_t page2;   // Page 2 Register
        } ne2000;
        
        uint8_t raw[16];     // Raw register access
    } m_registers;
    
    // Current register page (for NE2000)
    uint8_t m_currentPage;
    
    // Receive callback
    std::function<void(const uint8_t*, uint32_t)> m_receiveCallback;
    
    // Background packet processing thread
    std::thread m_packetThread;
    std::atomic<bool> m_threadRunning;
    std::mutex m_packetMutex;
    
    // Helper methods
    void initializeNE2000();
    void initializeRTL8139();
    void processPacketQueue();
    void packetThreadFunction();
    void setActivity(bool transmit, bool state);
    AdapterType adapterTypeFromString(const std::string& typeStr);
};

#endif // X86EMULATOR_NETWORK_ADAPTER_H