#pragma once

#include "x86emulator/network_device.h"
#include <array>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace x86emu {

/**
 * @brief RTL8139 network adapter implementation ported from 86Box.
 * 
 * This class implements the Realtek RTL8139 Fast Ethernet adapter,
 * providing 10/100 Mbps network connectivity with full PCI support.
 */
class RTL8139 : public NetworkDevice {
public:
    /**
     * Create an RTL8139 network adapter.
     * @param name Unique device name
     */
    explicit RTL8139(const std::string& name);
    
    /**
     * Destructor.
     */
    ~RTL8139() override;

    // Device interface implementation
    bool Initialize() override;
    void Reset() override;
    void Update(uint32_t deltaTime) override;
    
    // NetworkDevice interface implementation
    std::string GetMACAddress() const override;
    bool SetMACAddress(const std::string& mac) override;
    bool IsConnected() const override;
    bool Connect(const std::string& networkName) override;
    bool Disconnect() override;
    bool SendPacket(const uint8_t* data, size_t size) override;
    void RegisterPacketCallback(PacketCallback callback, void* userData) override;
    int GetLinkSpeed() const override;
    bool SetPromiscuousMode(bool enable) override;
    bool IsPromiscuousModeEnabled() const override;
    
    /**
     * Process a PCI reset.
     */
    void PCIReset();
    
    /**
     * Read from a device register.
     * @param address Register address
     * @param size Size of the read (1, 2, or 4 bytes)
     * @return Value read from the register
     */
    uint32_t ReadRegister(uint16_t address, int size);
    
    /**
     * Write to a device register.
     * @param address Register address
     * @param value Value to write
     * @param size Size of the write (1, 2, or 4 bytes)
     */
    void WriteRegister(uint16_t address, uint32_t value, int size);
    
    /**
     * Set the IRQ line number.
     * @param irq IRQ line number
     */
    void SetIRQ(int irq);
    
private:
    // RTL8139 register offsets
    enum RegisterOffsets {
        REG_MAC         = 0x00,  // MAC address
        REG_MAR         = 0x08,  // Multicast address registers
        REG_TXSTATUS0   = 0x10,  // Transmit status register 0
        REG_TXADDR0     = 0x20,  // Transmit address register 0
        REG_RXBUF       = 0x30,  // Receive buffer start address
        REG_CMD         = 0x37,  // Command register
        REG_CAPR        = 0x38,  // Current address of packet read
        REG_IMR         = 0x3C,  // Interrupt mask register
        REG_ISR         = 0x3E,  // Interrupt status register
        REG_TCR         = 0x40,  // Transmit configuration register
        REG_RCR         = 0x44,  // Receive configuration register
        REG_TCTR        = 0x48,  // Timer count register
        REG_MPC         = 0x4C,  // Missed packet counter
        REG_CONFIG0     = 0x51,  // Configuration register 0
        REG_CONFIG1     = 0x52,  // Configuration register 1
        REG_CONFIG3     = 0x59,  // Configuration register 3
        REG_CONFIG4     = 0x5A,  // Configuration register 4
        REG_MULINT      = 0x5C,  // Multiple interrupt select
        REG_RERID       = 0x5E,  // PCI revision ID
        REG_TSAD        = 0x60,  // Transmit status of all descriptors
        REG_BMCR        = 0x62,  // Basic mode control register
        REG_BMSR        = 0x64,  // Basic mode status register
        REG_ANAR        = 0x66,  // Auto-negotiation advertisement register
        REG_ANLPAR      = 0x68,  // Auto-negotiation link partner ability register
        REG_ANER        = 0x6A,  // Auto-negotiation expansion register
        REG_DIS         = 0x6C,  // Disconnect counter
        REG_FCSC        = 0x6E,  // False carrier sense counter
        REG_NWAYTR      = 0x70,  // N-way test register
        REG_REC         = 0x72,  // RX_ER counter
        REG_CSCR        = 0x74,  // CS configuration register
        REG_PHY1_PARM   = 0x78,  // PHY parameter 1
        REG_TW_PARM     = 0x7C,  // Twister parameter
        REG_PHY2_PARM   = 0x80,  // PHY parameter 2
        REG_CRC0        = 0x84,  // Power management CRC register
        REG_CRC1        = 0x85,  // Power management CRC register
        REG_WAKEUP0     = 0x8C,  // Power management wakeup frame
        REG_WAKEUP1     = 0x94,  // Power management wakeup frame
        REG_WAKEUP2     = 0x9C,  // Power management wakeup frame
        REG_WAKEUP3     = 0xA4,  // Power management wakeup frame
        REG_WAKEUP4     = 0xAC,  // Power management wakeup frame
        REG_WAKEUP5     = 0xB4,  // Power management wakeup frame
        REG_WAKEUP6     = 0xBC,  // Power management wakeup frame
        REG_WAKEUP7     = 0xC4,  // Power management wakeup frame
        REG_FLASH       = 0xD4,  // Flash memory read/write
        REG_CONFIG5     = 0xD8,  // Configuration register 5
        REG_FER         = 0xF0,  // Function event register
        REG_FEMR        = 0xF4,  // Function event mask register
        REG_FPSR        = 0xF8,  // Function present state register
        REG_FFER        = 0xFC   // Function force event register
    };
    
    // RTL8139 register bit definitions
    
    // Command register (REG_CMD)
    enum CommandBits {
        CMD_RESET       = 0x10,  // Reset
        CMD_RX_ENABLE   = 0x08,  // Receiver enable
        CMD_TX_ENABLE   = 0x04,  // Transmitter enable
        CMD_RX_BUF_EMPTY = 0x01  // Receive buffer empty
    };
    
    // Interrupt status/mask registers (REG_ISR, REG_IMR)
    enum InterruptBits {
        INT_PCIERR      = 0x8000,  // PCI bus error
        INT_PCS_TIMEOUT = 0x4000,  // PCS timeout
        INT_CABLE_LEN   = 0x2000,  // Cable length change
        INT_RX_OVERFLOW = 0x0010,  // Receive FIFO overflow
        INT_PACKET_UNDERRUN = 0x0008, // Packet underrun / link change
        INT_RX_FIFO_OVERFLOW = 0x0004, // Rx FIFO overflow
        INT_TX_OK       = 0x0002,  // Transmit OK
        INT_RX_OK       = 0x0001   // Receive OK
    };
    
    // Transmit configuration register (REG_TCR)
    enum TransmitConfigBits {
        TCR_HWVERID     = 0x7CC00000,  // Hardware version ID
        TCR_IFG         = 0x03000000,  // Interframe gap
        TCR_LBK         = 0x00060000,  // Loopback test
        TCR_CRC         = 0x00010000,  // Append CRC
        TCR_MXDMA       = 0x00000700,  // Max DMA burst size
        TCR_TXRR        = 0x000000F0,  // Tx retry count
        TCR_CLRABT      = 0x00000001   // Clear abort
    };
    
    // Receive configuration register (REG_RCR)
    enum ReceiveConfigBits {
        RCR_ERTH        = 0x0F000000,  // Early Rx threshold
        RCR_MRINT       = 0x00000040,  // Multiple early interrupt select
        RCR_RER8        = 0x00000020,  // Receive error packets > 8 bytes
        RCR_RXFTH       = 0x0000001C,  // Rx FIFO threshold
        RCR_RBLEN       = 0x00000003,  // Rx buffer length
        RCR_MXDMA       = 0x00000700,  // Max DMA burst size
        RCR_WRAP        = 0x00000080,  // Rx buffer wrap control
        RCR_AB          = 0x00000008,  // Accept broadcast packets
        RCR_AM          = 0x00000004,  // Accept multicast packets
        RCR_APM         = 0x00000002,  // Accept physical match packets
        RCR_AAP         = 0x00000001   // Accept all packets (promiscuous)
    };
    
    // Basic mode control register (REG_BMCR)
    enum BMCRBits {
        BMCR_RESET      = 0x8000,  // PHY reset
        BMCR_SPEED      = 0x2000,  // Speed selection (100Mbps)
        BMCR_ANE        = 0x1000,  // Auto-negotiation enable
        BMCR_RESTART    = 0x0200,  // Restart auto-negotiation
        BMCR_DUPLEX     = 0x0100   // Duplex mode (full duplex)
    };
    
    // Basic mode status register (REG_BMSR)
    enum BMSRBits {
        BMSR_ANEG_COMPLETE = 0x0020,  // Auto-negotiation complete
        BMSR_LINK_STATUS  = 0x0004    // Link status
    };
    
    // RTL8139 internal state
    
    // MAC address (6 bytes)
    std::array<uint8_t, 6> m_macAddress;
    
    // Connection status
    bool m_connected;
    
    // Link speed (10 or 100 Mbps)
    int m_linkSpeed;
    
    // Promiscuous mode
    bool m_promiscuousMode;
    
    // IRQ number
    int m_irq;
    
    // Packet callback
    PacketCallback m_packetCallback;
    void* m_callbackUserData;
    
    // 64KB receive buffer
    std::vector<uint8_t> m_rxBuffer;
    
    // Transmit buffers (4 buffers of 2KB each)
    std::array<std::vector<uint8_t>, 4> m_txBuffers;
    
    // Registers
    uint32_t m_txConfig;      // Transmit configuration
    uint32_t m_rxConfig;      // Receive configuration
    uint16_t m_interruptMask; // Interrupt mask
    uint16_t m_interruptStatus; // Interrupt status
    uint8_t m_command;        // Command register
    uint16_t m_capr;          // Current address of packet read
    uint16_t m_cbr;           // Current buffer address
    std::array<uint32_t, 4> m_txAddr; // Transmit address registers
    std::array<uint32_t, 4> m_txStatus; // Transmit status registers
    uint16_t m_txStatusAll;   // Transmit status of all descriptors
    uint16_t m_bmcr;          // Basic mode control register
    uint16_t m_bmsr;          // Basic mode status register
    uint16_t m_anar;          // Auto-negotiation advertisement register
    uint16_t m_anlpar;        // Auto-negotiation link partner ability register
    uint16_t m_aner;          // Auto-negotiation expansion register
    uint8_t m_config0;        // Configuration register 0
    uint8_t m_config1;        // Configuration register 1
    
    // Multicast address register (8 bytes)
    std::array<uint8_t, 8> m_mar;
    
    // Access synchronization
    mutable std::mutex m_accessMutex;
    
    // Received packet queue for processing
    std::queue<std::vector<uint8_t>> m_packetQueue;
    
    // Internal methods
    
    // Trigger an interrupt
    void TriggerInterrupt(uint16_t cause);
    
    // Check if interrupts are enabled for a specific cause
    bool IsInterruptEnabled(uint16_t cause) const;
    
    // Process a received packet
    void ProcessReceivedPacket(const uint8_t* data, size_t size);
    
    // Process a packet transmission
    void ProcessTransmit(int txIndex);
    
    // Update the auto-negotiation state
    void UpdateAutoNegotiation();
    
    // Generate a random MAC address
    void GenerateRandomMAC();
    
    // Helper to parse MAC address string
    bool ParseMACAddress(const std::string& mac, std::array<uint8_t, 6>& result) const;
    
    // Helper to format MAC address as string
    std::string FormatMACAddress(const std::array<uint8_t, 6>& mac) const;
};

} // namespace x86emu