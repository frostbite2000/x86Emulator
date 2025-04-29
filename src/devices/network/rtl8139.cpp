#include "x86emulator/network/rtl8139.h"
#include "x86emulator/io.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>

namespace x86emu {

RTL8139::RTL8139(const std::string& name)
    : NetworkDevice(name, "Realtek RTL8139 Fast Ethernet", NetworkAdapterType::ETHERNET)
    , m_connected(false)
    , m_linkSpeed(100)  // Default to 100 Mbps
    , m_promiscuousMode(false)
    , m_irq(10)  // Default IRQ
    , m_packetCallback(nullptr)
    , m_callbackUserData(nullptr)
    , m_txConfig(0)
    , m_rxConfig(0)
    , m_interruptMask(0)
    , m_interruptStatus(0)
    , m_command(0)
    , m_capr(0)
    , m_cbr(0)
    , m_txStatusAll(0)
    , m_bmcr(0)
    , m_bmsr(0)
    , m_anar(0)
    , m_anlpar(0)
    , m_aner(0)
    , m_config0(0)
    , m_config1(0)
{
    // Initialize MAC address
    GenerateRandomMAC();
    
    // Initialize receive buffer (64KB)
    m_rxBuffer.resize(64 * 1024);
    std::fill(m_rxBuffer.begin(), m_rxBuffer.end(), 0);
    
    // Initialize transmit buffers (4 buffers of 2KB each)
    for (auto& buffer : m_txBuffers) {
        buffer.resize(2 * 1024);
        std::fill(buffer.begin(), buffer.end(), 0);
    }
    
    // Initialize transmit addresses and status
    m_txAddr.fill(0);
    m_txStatus.fill(0);
    
    // Initialize multicast address register
    m_mar.fill(0);
    
    // Set default BMSR (link up, auto-negotiation complete)
    m_bmsr = BMSR_LINK_STATUS | BMSR_ANEG_COMPLETE;
}

RTL8139::~RTL8139()
{
    // Ensure we're disconnected
    Disconnect();
}

bool RTL8139::Initialize()
{
    if (IsInitialized()) {
        return true;
    }
    
    // Perform a hardware reset
    Reset();
    
    // Set initialized state
    SetInitialized(true);
    
    return true;
}

void RTL8139::Reset()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Clear internal state
    m_txConfig = 0;
    m_rxConfig = 0;
    m_interruptMask = 0;
    m_interruptStatus = 0;
    m_command = 0;
    m_capr = 0;
    m_cbr = 0;
    m_txStatusAll = 0;
    
    // Reset buffers
    std::fill(m_rxBuffer.begin(), m_rxBuffer.end(), 0);
    
    for (auto& buffer : m_txBuffers) {
        std::fill(buffer.begin(), buffer.end(), 0);
    }
    
    // Reset transmit addresses and status
    m_txAddr.fill(0);
    m_txStatus.fill(0);
    
    // Reset multicast address register
    m_mar.fill(0);
    
    // Set default PHY registers
    m_bmcr = 0;
    m_bmsr = BMSR_LINK_STATUS | BMSR_ANEG_COMPLETE; // Link up, auto-negotiation complete
    m_anar = 0;
    m_anlpar = 0;
    m_aner = 0;
    
    // Clear packet queue
    while (!m_packetQueue.empty()) {
        m_packetQueue.pop();
    }
    
    // Set default configuration
    m_config0 = 0;
    m_config1 = 0;
}

void RTL8139::Update(uint32_t deltaTime)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Process any packets in the queue
    while (!m_packetQueue.empty()) {
        auto& packet = m_packetQueue.front();
        ProcessReceivedPacket(packet.data(), packet.size());
        m_packetQueue.pop();
    }
    
    // Update auto-negotiation if needed
    UpdateAutoNegotiation();
}

std::string RTL8139::GetMACAddress() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return FormatMACAddress(m_macAddress);
}

bool RTL8139::SetMACAddress(const std::string& mac)
{
    std::array<uint8_t, 6> newMac;
    if (!ParseMACAddress(mac, newMac)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_macAddress = newMac;
    
    return true;
}

bool RTL8139::IsConnected() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_connected;
}

bool RTL8139::Connect(const std::string& networkName)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Set connected state
    m_connected = true;
    
    // Update BMSR to show link is up
    m_bmsr |= BMSR_LINK_STATUS;
    
    return true;
}

bool RTL8139::Disconnect()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Set disconnected state
    m_connected = false;
    
    // Update BMSR to show link is down
    m_bmsr &= ~BMSR_LINK_STATUS;
    
    return true;
}

bool RTL8139::SendPacket(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return false;
    }
    
    if (m_packetCallback && m_connected) {
        // Call the packet callback
        m_packetCallback(data, size, m_callbackUserData);
    }
    
    return true;
}

void RTL8139::RegisterPacketCallback(PacketCallback callback, void* userData)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_packetCallback = callback;
    m_callbackUserData = userData;
}

int RTL8139::GetLinkSpeed() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_linkSpeed;
}

bool RTL8139::SetPromiscuousMode(bool enable)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    m_promiscuousMode = enable;
    
    // Update the receive configuration register
    if (enable) {
        m_rxConfig |= RCR_AAP;  // Accept all packets
    } else {
        m_rxConfig &= ~RCR_AAP; // Normal filtering
    }
    
    return true;
}

bool RTL8139::IsPromiscuousModeEnabled() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_promiscuousMode;
}

void RTL8139::PCIReset()
{
    Reset();
}

uint32_t RTL8139::ReadRegister(uint16_t address, int size)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    uint32_t value = 0;
    
    switch (address) {
        case REG_MAC:
        case REG_MAC + 1:
        case REG_MAC + 2:
        case REG_MAC + 3:
        case REG_MAC + 4:
        case REG_MAC + 5:
            // MAC address
            if (size == 1) {
                value = m_macAddress[address - REG_MAC];
            } else if (size == 2 && (address % 2) == 0) {
                value = static_cast<uint16_t>(m_macAddress[address - REG_MAC]) |
                        (static_cast<uint16_t>(m_macAddress[address - REG_MAC + 1]) << 8);
            } else if (size == 4 && (address % 4) == 0) {
                value = static_cast<uint32_t>(m_macAddress[address - REG_MAC]) |
                        (static_cast<uint32_t>(m_macAddress[address - REG_MAC + 1]) << 8) |
                        (static_cast<uint32_t>(m_macAddress[address - REG_MAC + 2]) << 16) |
                        (static_cast<uint32_t>(m_macAddress[address - REG_MAC + 3]) << 24);
            }
            break;
        
        case REG_MAR:
        case REG_MAR + 1:
        case REG_MAR + 2:
        case REG_MAR + 3:
        case REG_MAR + 4:
        case REG_MAR + 5:
        case REG_MAR + 6:
        case REG_MAR + 7:
            // Multicast address register
            if (size == 1) {
                value = m_mar[address - REG_MAR];
            } else if (size == 2 && (address % 2) == 0) {
                value = static_cast<uint16_t>(m_mar[address - REG_MAR]) |
                        (static_cast<uint16_t>(m_mar[address - REG_MAR + 1]) << 8);
            } else if (size == 4 && (address % 4) == 0) {
                value = static_cast<uint32_t>(m_mar[address - REG_MAR]) |
                        (static_cast<uint32_t>(m_mar[address - REG_MAR + 1]) << 8) |
                        (static_cast<uint32_t>(m_mar[address - REG_MAR + 2]) << 16) |
                        (static_cast<uint32_t>(m_mar[address - REG_MAR + 3]) << 24);
            }
            break;
        
        case REG_TXSTATUS0:
        case REG_TXSTATUS0 + 4:
        case REG_TXSTATUS0 + 8:
        case REG_TXSTATUS0 + 12:
            // Transmit status registers
            if (size == 4) {
                int index = (address - REG_TXSTATUS0) / 4;
                value = m_txStatus[index];
            }
            break;
        
        case REG_TXADDR0:
        case REG_TXADDR0 + 4:
        case REG_TXADDR0 + 8:
        case REG_TXADDR0 + 12:
            // Transmit address registers
            if (size == 4) {
                int index = (address - REG_TXADDR0) / 4;
                value = m_txAddr[index];
            }
            break;
        
        case REG_RXBUF:
            // Receive buffer start address
            if (size == 4) {
                value = 0; // This is a write-only register
            }
            break;
        
        case REG_CMD:
            // Command register
            if (size == 1) {
                value = m_command;
            }
            break;
        
        case REG_CAPR:
            // Current address of packet read
            if (size == 2) {
                value = m_capr;
            }
            break;
        
        case REG_IMR:
            // Interrupt mask register
            if (size == 2) {
                value = m_interruptMask;
            }
            break;
        
        case REG_ISR:
            // Interrupt status register
            if (size == 2) {
                value = m_interruptStatus;
            }
            break;
        
        case REG_TCR:
            // Transmit configuration register
            if (size == 4) {
                value = m_txConfig;
            }
            break;
        
        case REG_RCR:
            // Receive configuration register
            if (size == 4) {
                value = m_rxConfig;
            }
            break;
        
        case REG_TSAD:
            // Transmit status of all descriptors
            if (size == 2) {
                value = m_txStatusAll;
            }
            break;
        
        case REG_BMCR:
            // Basic mode control register
            if (size == 2) {
                value = m_bmcr;
            }
            break;
        
        case REG_BMSR:
            // Basic mode status register
            if (size == 2) {
                value = m_bmsr;
            }
            break;
        
        case REG_ANAR:
            // Auto-negotiation advertisement register
            if (size == 2) {
                value = m_anar;
            }
            break;
        
        case REG_ANLPAR:
            // Auto-negotiation link partner ability register
            if (size == 2) {
                value = m_anlpar;
            }
            break;
        
        case REG_ANER:
            // Auto-negotiation expansion register
            if (size == 2) {
                value = m_aner;
            }
            break;
        
        case REG_CONFIG0:
            // Configuration register 0
            if (size == 1) {
                value = m_config0;
            }
            break;
        
        case REG_CONFIG1:
            // Configuration register 1
            if (size == 1) {
                value = m_config1;
            }
            break;
    }
    
    return value;
}

void RTL8139::WriteRegister(uint16_t address, uint32_t value, int size)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    switch (address) {
        case REG_MAC:
        case REG_MAC + 1:
        case REG_MAC + 2:
        case REG_MAC + 3:
        case REG_MAC + 4:
        case REG_MAC + 5:
            // MAC address
            if (size == 1) {
                m_macAddress[address - REG_MAC] = static_cast<uint8_t>(value);
            } else if (size == 2 && (address % 2) == 0) {
                m_macAddress[address - REG_MAC] = static_cast<uint8_t>(value & 0xFF);
                m_macAddress[address - REG_MAC + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
            } else if (size == 4 && (address % 4) == 0) {
                m_macAddress[address - REG_MAC] = static_cast<uint8_t>(value & 0xFF);
                m_macAddress[address - REG_MAC + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
                m_macAddress[address - REG_MAC + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
                m_macAddress[address - REG_MAC + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
            }
            break;
        
        case REG_MAR:
        case REG_MAR + 1:
        case REG_MAR + 2:
        case REG_MAR + 3:
        case REG_MAR + 4:
        case REG_MAR + 5:
        case REG_MAR + 6:
        case REG_MAR + 7:
            // Multicast address register
            if (size == 1) {
                m_mar[address - REG_MAR] = static_cast<uint8_t>(value);
            } else if (size == 2 && (address % 2) == 0) {
                m_mar[address - REG_MAR] = static_cast<uint8_t>(value & 0xFF);
                m_mar[address - REG_MAR + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
            } else if (size == 4 && (address % 4) == 0) {
                m_mar[address - REG_MAR] = static_cast<uint8_t>(value & 0xFF);
                m_mar[address - REG_MAR + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
                m_mar[address - REG_MAR + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
                m_mar[address - REG_MAR + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
            }
            break;
        
        case REG_TXADDR0:
        case REG_TXADDR0 + 4:
        case REG_TXADDR0 + 8:
        case REG_TXADDR0 + 12:
            // Transmit address registers
            if (size == 4) {
                int index = (address - REG_TXADDR0) / 4;
                m_txAddr[index] = value;
                // Process transmit when the address is written
                ProcessTransmit(index);
            }
            break;
        
        case REG_RXBUF:
            // Receive buffer start address
            if (size == 4) {
                // RTL8139 uses physical addresses, but we're only interested in the offset
                // Reset the CAPR and CBR registers
                m_capr = 0;
                m_cbr = 0;
            }
            break;
        
        case REG_CMD:
            // Command register
            if (size == 1) {
                uint8_t oldCommand = m_command;
                m_command = static_cast<uint8_t>(value);
                
                // Check for reset command
                if ((value & CMD_RESET) && !(oldCommand & CMD_RESET)) {
                    Reset();
                }
            }
            break;
        
        case REG_CAPR:
            // Current address of packet read
            if (size == 2) {
                m_capr = static_cast<uint16_t>(value);
            }
            break;
        
        case REG_IMR:
            // Interrupt mask register
            if (size == 2) {
                m_interruptMask = static_cast<uint16_t>(value);
                
                // If any interrupts are now enabled that were previously triggered,
                // re-trigger them
                if (m_interruptStatus & m_interruptMask) {
                    // Call the IRQ handler (PCI bus would handle this)
                }
            }
            break;
        
        case REG_ISR:
            // Interrupt status register
            if (size == 2) {
                // Writing 1 to a bit clears that interrupt
                m_interruptStatus &= ~(static_cast<uint16_t>(value));
            }
            break;
        
        case REG_TCR:
            // Transmit configuration register
            if (size == 4) {
                m_txConfig = value;
            }
            break;
        
        case REG_RCR:
            // Receive configuration register
            if (size == 4) {
                m_rxConfig = value;
                
                // Update promiscuous mode based on AAP bit
                m_promiscuousMode = ((value & RCR_AAP) != 0);
            }
            break;
        
        case REG_BMCR:
            // Basic mode control register
            if (size == 2) {
                m_bmcr = static_cast<uint16_t>(value);
                
                // Check for PHY reset
                if (value & BMCR_RESET) {
                    m_bmcr &= ~BMCR_RESET; // Self-clearing bit
                    
                    // Reset the PHY registers to defaults
                    m_anar = 0;
                    m_anlpar = 0;
                    m_aner = 0;
                }
                
                // Update link speed based on BMCR_SPEED bit
                m_linkSpeed = (value & BMCR_SPEED) ? 100 : 10;
                
                // If auto-negotiation is being restarted, update the status
                if (value & BMCR_RESTART) {
                    m_bmcr &= ~BMCR_RESTART; // Self-clearing bit
                    UpdateAutoNegotiation();
                }
            }
            break;
        
        case REG_ANAR:
            // Auto-negotiation advertisement register
            if (size == 2) {
                m_anar = static_cast<uint16_t>(value);
                
                // If auto-negotiation is enabled, update the status
                if (m_bmcr & BMCR_ANE) {
                    UpdateAutoNegotiation();
                }
            }
            break;
        
        case REG_CONFIG0:
            // Configuration register 0
            if (size == 1) {
                m_config0 = static_cast<uint8_t>(value);
            }
            break;
        
        case REG_CONFIG1:
            // Configuration register 1
            if (size == 1) {
                m_config1 = static_cast<uint8_t>(value);
            }
            break;
    }
}

void RTL8139::SetIRQ(int irq)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_irq = irq;
}

void RTL8139::TriggerInterrupt(uint16_t cause)
{
    // Set the interrupt status bit
    m_interruptStatus |= cause;
    
    // Check if this interrupt is enabled
    if (IsInterruptEnabled(cause)) {
        // Call the IRQ handler (PCI bus would handle this)
        // In a real implementation, this would raise the IRQ line
    }
}

bool RTL8139::IsInterruptEnabled(uint16_t cause) const
{
    return (m_interruptMask & cause) != 0;
}

void RTL8139::ProcessReceivedPacket(const uint8_t* data, size_t size)
{
    // Check if receiver is enabled
    if (!(m_command & CMD_RX_ENABLE)) {
        return;
    }
    
    // Check packet size
    if (size == 0 || size > 1500) {
        return;
    }
    
    // Check if we should accept this packet based on RCR
    bool acceptPacket = false;
    
    // Check destination MAC address
    if (data[0] == 0xFF && data[1] == 0xFF && data[2] == 0xFF && 
        data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF) {
        // Broadcast packet
        if (m_rxConfig & RCR_AB) {
            acceptPacket = true;
        }
    } else if ((data[0] & 0x01) != 0) {
        // Multicast packet
        if (m_rxConfig & RCR_AM) {
            acceptPacket = true;
        }
    } else if (data[0] == m_macAddress[0] && data[1] == m_macAddress[1] && 
               data[2] == m_macAddress[2] && data[3] == m_macAddress[3] && 
               data[4] == m_macAddress[4] && data[5] == m_macAddress[5]) {
        // Unicast packet for our MAC
        if (m_rxConfig & RCR_APM) {
            acceptPacket = true;
        }
    } else if (m_rxConfig & RCR_AAP) {
        // Promiscuous mode - accept all packets
        acceptPacket = true;
    }
    
    if (!acceptPacket) {
        return;
    }
    
    // Calculate the total size including the packet header
    // Packet format: Status (2) + Length (2) + Data + Padding (0-3)
    size_t totalSize = size + 4;
    
    // Add padding to ensure dword alignment
    size_t paddingBytes = (4 - (totalSize % 4)) % 4;
    totalSize += paddingBytes;
    
    // Check if there's enough space in the buffer
    if (m_cbr + totalSize > m_rxBuffer.size()) {
        // Not enough space - check if wrap is enabled
        if (m_rxConfig & RCR_WRAP) {
            // Wrap around
            m_cbr = 0;
        } else {
            // Buffer overflow - set interrupt
            TriggerInterrupt(INT_RX_OVERFLOW);
            return;
        }
    }
    
    // Construct the packet header
    // Status: bit 0 = ROK, bits 13:16 = MAR, bits 29:31 = Copy all bytes
    uint16_t status = 0x0001;  // ROK (Receive OK)
    
    // Write the header to the buffer
    m_rxBuffer[m_cbr] = status & 0xFF;
    m_rxBuffer[m_cbr + 1] = (status >> 8) & 0xFF;
    m_rxBuffer[m_cbr + 2] = size & 0xFF;
    m_rxBuffer[m_cbr + 3] = (size >> 8) & 0xFF;
    
    // Write the packet data
    std::memcpy(&m_rxBuffer[m_cbr + 4], data, size);
    
    // Add padding bytes if needed
    for (size_t i = 0; i < paddingBytes; ++i) {
        m_rxBuffer[m_cbr + 4 + size + i] = 0;
    }
    
    // Update the buffer pointer
    m_cbr = (m_cbr + totalSize) % m_rxBuffer.size();
    
    // Set the CMD_RX_BUF_EMPTY bit to 0
    m_command &= ~CMD_RX_BUF_EMPTY;
    
    // Trigger an interrupt
    TriggerInterrupt(INT_RX_OK);
}

void RTL8139::ProcessTransmit(int txIndex)
{
    // Check if transmitter is enabled
    if (!(m_command & CMD_TX_ENABLE)) {
        return;
    }
    
    // Get the physical address and size of the packet
    uint32_t txAddr = m_txAddr[txIndex];
    
    // In a real implementation, we would DMA the data from the physical address
    // For our simulation, we'll assume we have access to the data
    
    // Placeholder - assume the packet is in the tx buffer
    const uint8_t* packetData = m_txBuffers[txIndex].data();
    
    // Get the size - in a real implementation this would come from the memory controller
    // For now, we'll assume a 1KB packet
    size_t packetSize = 1024;
    
    // Send the packet to the network
    if (SendPacket(packetData, packetSize)) {
        // Set the transmit status
        m_txStatus[txIndex] = (packetSize & 0x0FFF) | 0x2000;  // Transmit OK
        
        // Update the transmit status of all descriptors
        m_txStatusAll |= (1 << txIndex);
        
        // Trigger an interrupt
        TriggerInterrupt(INT_TX_OK);
    } else {
        // Transmit failed
        m_txStatus[txIndex] = 0x8000;  // Transmit error
    }
}

void RTL8139::UpdateAutoNegotiation()
{
    // If auto-negotiation is enabled
    if (m_bmcr & BMCR_ANE) {
        // Set the auto-negotiation complete bit
        m_bmsr |= BMSR_ANEG_COMPLETE;
        
        // Set the link partner ability register based on our advertisement
        m_anlpar = m_anar & 0x3F0F;  // Support only what we advertise
        
        // If the speed bit is set, advertise 100Mbps
        if (m_bmcr & BMCR_SPEED) {
            m_linkSpeed = 100;
        } else {
            m_linkSpeed = 10;
        }
        
        // If the duplex bit is set, set full duplex mode
        // (This doesn't affect our simulation for now)
    }
}

void RTL8139::GenerateRandomMAC()
{
    // Use current time to seed the random number generator
    std::default_random_engine generator(static_cast<unsigned int>(std::time(0)));
    std::uniform_int_distribution<int> distribution(0, 255);
    
    // Generate a unique MAC address with a vendor ID prefix
    // Use Realtek's vendor prefix (00:E0:4C)
    m_macAddress[0] = 0x00;
    m_macAddress[1] = 0xE0;
    m_macAddress[2] = 0x4C;
    
    // Generate random values for the remaining bytes
    for (int i = 3; i < 6; ++i) {
        m_macAddress[i] = static_cast<uint8_t>(distribution(generator));
    }
}

bool RTL8139::ParseMACAddress(const std::string& mac, std::array<uint8_t, 6>& result) const
{
    // Parse MAC address string (e.g., "00:11:22:33:44:55")
    std::istringstream ss(mac);
    std::string octet;
    int i = 0;
    
    while (std::getline(ss, octet, ':')) {
        if (i >= 6) {
            return false;  // Too many octets
        }
        
        try {
            result[i++] = static_cast<uint8_t>(std::stoi(octet, nullptr, 16));
        } catch (...) {
            return false;  // Invalid octet
        }
    }
    
    return i == 6;  // Must have exactly 6 octets
}

std::string RTL8139::FormatMACAddress(const std::array<uint8_t, 6>& mac) const
{
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 6; ++i) {
        if (i > 0) {
            ss << ':';
        }
        ss << std::setw(2) << static_cast<int>(mac[i]);
    }
    
    return ss.str();
}

} // namespace x86emu