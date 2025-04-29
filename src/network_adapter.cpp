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

#include "network_adapter.h"
#include "logger.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

// NE2000 I/O ports
constexpr uint16_t NE2000_BASE_PORT = 0x300;
constexpr int NE2000_MEM_SIZE = 32 * 1024;

NetworkAdapter::NetworkAdapter(const std::string& cardType)
    : Device("NetworkAdapter_" + cardType),
      m_adapterType(adapterTypeFromString(cardType)),
      m_cardType(cardType),
      m_enabled(false),
      m_promiscuousMode(false),
      m_multicastMode(false),
      m_transmitActive(false),
      m_receiveActive(false),
      m_activityTimeout(0),
      m_memorySize(0),
      m_currentPage(0),
      m_threadRunning(false)
{
    // Generate a random MAC address
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 255);
    
    // First byte: locally administered address bit set
    m_macAddress[0] = 0x02;  // Locally administered, unicast
    
    // Random rest of the address
    for (int i = 1; i < 6; i++) {
        m_macAddress[i] = static_cast<uint8_t>(distrib(gen));
    }
    
    // Set up base IO port
    m_baseIOPort = NE2000_BASE_PORT;
    
    // Clear registers
    std::memset(&m_registers, 0, sizeof(m_registers));
}

NetworkAdapter::~NetworkAdapter()
{
    // Stop packet processing thread
    if (m_threadRunning) {
        m_threadRunning = false;
        if (m_packetThread.joinable()) {
            m_packetThread.join();
        }
    }
    
    // Clear packet queues
    std::lock_guard<std::mutex> lock(m_packetMutex);
    while (!m_receiveQueue.empty()) {
        m_receiveQueue.pop();
    }
    while (!m_transmitQueue.empty()) {
        m_transmitQueue.pop();
    }
}

bool NetworkAdapter::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Initialize based on adapter type
        switch (m_adapterType) {
            case AdapterType::NE2000:
                initializeNE2000();
                break;
            case AdapterType::RTL8139:
                initializeRTL8139();
                break;
            case AdapterType::PCAP:
                // Not implemented yet
                m_lastError = "PCAP network adapter not implemented";
                return false;
        }
        
        // Start packet processing thread
        m_threadRunning = true;
        m_packetThread = std::thread(&NetworkAdapter::packetThreadFunction, this);
        
        Logger::GetInstance()->info("Network adapter initialized: %s", m_cardType.c_str());
        Logger::GetInstance()->debug("MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
                               m_macAddress[0], m_macAddress[1], m_macAddress[2],
                               m_macAddress[3], m_macAddress[4], m_macAddress[5]);
        
        return true;
    }
    catch (const std::exception& ex) {
        m_lastError = ex.what();
        Logger::GetInstance()->error("Network adapter initialization failed: %s", ex.what());
        return false;
    }
}

void NetworkAdapter::update(int cycles)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update activity timeout
    if ((m_transmitActive || m_receiveActive) && m_activityTimeout > 0) {
        m_activityTimeout -= cycles;
        if (m_activityTimeout <= 0) {
            setActivity(false, false);
        }
    }
    
    // Process packet queue
    processPacketQueue();
}

void NetworkAdapter::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset state
    m_enabled = false;
    m_promiscuousMode = false;
    m_multicastMode = false;
    
    // Reset activity
    setActivity(false, false);
    
    // Clear registers
    std::memset(&m_registers, 0, sizeof(m_registers));
    
    // Reset current page
    m_currentPage = 0;
    
    // Clear packet queues
    std::lock_guard<std::mutex> packetLock(m_packetMutex);
    while (!m_receiveQueue.empty()) {
        m_receiveQueue.pop();
    }
    while (!m_transmitQueue.empty()) {
        m_transmitQueue.pop();
    }
    
    Logger::GetInstance()->info("Network adapter reset");
}

uint8_t NetworkAdapter::readRegister(uint16_t port) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Calculate register index
    uint16_t regIndex = port - m_baseIOPort;
    
    // Check if register is valid
    if (regIndex >= sizeof(m_registers.raw)) {
        Logger::GetInstance()->debug("Read from invalid network register: 0x%04X", port);
        return 0xFF;
    }
    
    // NE2000 specific register handling
    if (m_adapterType == AdapterType::NE2000) {
        switch (regIndex) {
            case 0x00: // Command Register
                return m_registers.ne2000.cr;
            
            case 0x07: // ISR - Interrupt Status Register
                return m_registers.ne2000.isr;
            
            // MAC address (00:00:00:00:00:00 at offsets 0x01-0x06 in page 0)
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
                if (m_currentPage == 0) {
                    return m_macAddress[regIndex - 1];
                }
                break;
        }
    }
    
    // Return raw register value
    return m_registers.raw[regIndex];
}

void NetworkAdapter::writeRegister(uint16_t port, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Calculate register index
    uint16_t regIndex = port - m_baseIOPort;
    
    // Check if register is valid
    if (regIndex >= sizeof(m_registers.raw)) {
        Logger::GetInstance()->debug("Write to invalid network register: 0x%04X = 0x%02X", port, value);
        return;
    }
    
    // Store raw register value
    m_registers.raw[regIndex] = value;
    
    // NE2000 specific register handling
    if (m_adapterType == AdapterType::NE2000) {
        switch (regIndex) {
            case 0x00: // Command Register
                m_registers.ne2000.cr = value;
                
                // Check if page selection changed
                m_currentPage = (value >> 6) & 0x03;
                
                // Check if reset bit set
                if (value & 0x01) {
                    reset();
                }
                
                // Check if start bit set
                if (value & 0x02) {
                    m_enabled = true;
                }
                
                // Check if stop bit set
                if (value & 0x04) {
                    m_enabled = false;
                }
                break;
            
            case 0x0F: // RCR - Receive Configuration Register
                m_registers.ne2000.rcr = value;
                
                // Check if promiscuous mode changed
                m_promiscuousMode = (value & 0x10) != 0;
                
                // Check if multicast mode changed
                m_multicastMode = (value & 0x08) != 0;
                break;
        }
    }
}

uint8_t NetworkAdapter::readMemory(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address >= m_memory.size()) {
        Logger::GetInstance()->debug("Read from invalid network memory address: 0x%08X", address);
        return 0xFF;
    }
    
    // Return memory value
    return m_memory[address];
}

void NetworkAdapter::writeMemory(uint32_t address, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is valid
    if (address >= m_memory.size()) {
        Logger::GetInstance()->debug("Write to invalid network memory address: 0x%08X = 0x%02X", address, value);
        return;
    }
    
    // Write memory value
    m_memory[address] = value;
}

bool NetworkAdapter::sendPacket(const uint8_t* data, uint32_t size)
{
    // Check parameters
    if (!data || size == 0 || size > 1600) {  // Typical Ethernet MTU is 1500
        return false;
    }
    
    // Create packet
    Packet packet;
    packet.data.resize(size);
    std::memcpy(packet.data.data(), data, size);
    packet.size = size;
    packet.broadcast = false;
    packet.multicast = false;
    
    // Check destination MAC for broadcast/multicast
    if (data[0] == 0xFF && data[1] == 0xFF && data[2] == 0xFF &&
        data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF) {
        packet.broadcast = true;
    } else if (data[0] & 0x01) {
        packet.multicast = true;
    }
    
    // Add to transmit queue
    std::lock_guard<std::mutex> lock(m_packetMutex);
    m_transmitQueue.push(packet);
    
    // Set transmit activity
    setActivity(true, true);
    
    return true;
}

uint32_t NetworkAdapter::receivePacket(uint8_t* data, uint32_t maxSize)
{
    std::lock_guard<std::mutex> lock(m_packetMutex);
    
    // Check if receive queue is empty
    if (m_receiveQueue.empty()) {
        return 0;
    }
    
    // Get packet from queue
    const Packet& packet = m_receiveQueue.front();
    
    // Check buffer size
    if (maxSize < packet.size) {
        return 0;
    }
    
    // Copy packet data to buffer
    std::memcpy(data, packet.data.data(), packet.size);
    uint32_t size = packet.size;
    
    // Remove packet from queue
    m_receiveQueue.pop();
    
    // Set receive activity
    setActivity(false, true);
    
    return size;
}

bool NetworkAdapter::isActive() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_transmitActive || m_receiveActive;
}

const std::array<uint8_t, 6>& NetworkAdapter::getMACAddress() const
{
    return m_macAddress;
}

void NetworkAdapter::setMACAddress(const std::array<uint8_t, 6>& mac)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_macAddress = mac;
}

NetworkAdapter::AdapterType NetworkAdapter::getAdapterType() const
{
    return m_adapterType;
}

void NetworkAdapter::setReceiveCallback(std::function<void(const uint8_t*, uint32_t)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receiveCallback = callback;
}

void NetworkAdapter::initializeNE2000()
{
    // Initialize NE2000 memory
    m_memorySize = NE2000_MEM_SIZE;
    m_memory.resize(m_memorySize, 0);
    
    // Initialize registers
    std::memset(&m_registers, 0, sizeof(m_registers));
    
    // Set up MAC address registers
    for (int i = 0; i < 6; i++) {
        m_registers.raw[i + 1] = m_macAddress[i];
    }
    
    // Set up initial register values
    m_registers.ne2000.cr = 0x01;  // Stopped, page 0
    m_registers.ne2000.isr = 0x00;  // No interrupts
    m_registers.ne2000.imr = 0x00;  // All interrupts masked
    m_registers.ne2000.rcr = 0x00;  // Normal receive mode
    m_registers.ne2000.tcr = 0x00;  // Normal transmit mode
    m_registers.ne2000.dcr = 0x48;  // 8-bit DMA, normal operation
    m_registers.ne2000.rsr = 0x00;  // No receive status
    
    // Set current page
    m_currentPage = 0;
}

void NetworkAdapter::initializeRTL8139()
{
    // Not implemented yet
    throw std::runtime_error("RTL8139 network adapter not implemented");
}

void NetworkAdapter::processPacketQueue()
{
    std::lock_guard<std::mutex> lock(m_packetMutex);
    
    // Process transmit queue
    while (!m_transmitQueue.empty()) {
        const Packet& packet = m_transmitQueue.front();
        
        // Call receive callback if defined
        if (m_receiveCallback) {
            m_receiveCallback(packet.data.data(), packet.size);
        }
        
        // Remove packet from queue
        m_transmitQueue.pop();
    }
}

void NetworkAdapter::packetThreadFunction()
{
    while (m_threadRunning) {
        // Sleep for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Process packet queue
        processPacketQueue();
    }
}

void NetworkAdapter::setActivity(bool transmit, bool state)
{
    if (transmit) {
        m_transmitActive = state;
    } else {
        m_receiveActive = state;
    }
    
    // Update activity flag based on either transmit or receive
    m_active = m_transmitActive || m_receiveActive;
    
    // Reset timeout
    if (m_active) {
        m_activityTimeout = 5000;  // 5,000 cycles
    } else {
        m_activityTimeout = 0;
    }
}

NetworkAdapter::AdapterType NetworkAdapter::adapterTypeFromString(const std::string& typeStr)
{
    std::string lowercaseType = typeStr;
    std::transform(lowercaseType.begin(), lowercaseType.end(), lowercaseType.begin(),
                  [](unsigned char c){ return std::tolower(c); });
    
    if (lowercaseType == "ne2000") {
        return AdapterType::NE2000;
    } else if (lowercaseType == "rtl8139") {
        return AdapterType::RTL8139;
    } else if (lowercaseType == "pcap") {
        return AdapterType::PCAP;
    } else {
        // Default to NE2000
        return AdapterType::NE2000;
    }
}