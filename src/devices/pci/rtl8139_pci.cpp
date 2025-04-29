#include "x86emulator/pci/rtl8139_pci.h"
#include <algorithm>
#include <cassert>

namespace x86emu {

// PCI Vendor and Device IDs for RTL8139
const uint16_t RTL8139_VENDOR_ID = 0x10EC;  // Realtek
const uint16_t RTL8139_DEVICE_ID = 0x8139;  // RTL8139

// PCI Class Code for Ethernet Controller
const uint8_t RTL8139_CLASS_CODE[3] = { 0x00, 0x00, 0x02 };  // Network, Ethernet

RTL8139PCI::RTL8139PCI(const std::string& name)
    : PCIDevice(name, "Realtek RTL8139 Fast Ethernet PCI")
    , m_vendorID(RTL8139_VENDOR_ID)
    , m_deviceID(RTL8139_DEVICE_ID)
    , m_command(0)
    , m_status(0)
    , m_revisionID(0x10)  // RTL8139C
    , m_headerType(0x00)  // Standard PCI header
    , m_interruptLine(0)
    , m_interruptPin(1)   // INTA
    , m_bar0(0)
    , m_bar1(0)
    , m_ioBase(0)
    , m_ioSize(256)       // RTL8139 uses 256 bytes of I/O space
    , m_memBase(0)
    , m_memSize(256)      // RTL8139 uses 256 bytes of memory space
{
    // Set class code
    m_classcode[0] = RTL8139_CLASS_CODE[0];
    m_classcode[1] = RTL8139_CLASS_CODE[1];
    m_classcode[2] = RTL8139_CLASS_CODE[2];
    
    // Create the RTL8139 adapter
    m_adapter = std::make_unique<RTL8139>(name + "_adapter");
}

RTL8139PCI::~RTL8139PCI()
{
    // Network adapter will be cleaned up by its unique_ptr
}

bool RTL8139PCI::Initialize()
{
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize the network adapter
    if (!m_adapter->Initialize()) {
        return false;
    }
    
    // Set initialized state
    SetInitialized(true);
    
    return true;
}

void RTL8139PCI::Reset()
{
    // Reset PCI configuration
    m_command = 0;
    m_status = 0;
    m_bar0 = 0;
    m_bar1 = 0;
    m_ioBase = 0;
    m_memBase = 0;
    
    // Reset the network adapter
    m_adapter->Reset();
}

void RTL8139PCI::Update(uint32_t deltaTime)
{
    // Update the network adapter
    m_adapter->Update(deltaTime);
}

uint32_t RTL8139PCI::ReadConfig(uint8_t reg, int size)
{
    uint32_t value = 0;
    
    switch (reg) {
        case 0x00:  // Vendor ID
            if (size >= 2) {
                value = m_vendorID;
            }
            break;
        
        case 0x02:  // Device ID
            if (size >= 2) {
                value = m_deviceID;
            }
            break;
            
        case 0x04:  // Command
            if (size >= 2) {
                value = m_command;
            }
            break;
            
        case 0x06:  // Status
            if (size >= 2) {
                value = m_status;
            }
            break;
            
        case 0x08:  // Revision ID
            if (size >= 1) {
                value = m_revisionID;
            }
            break;
            
        case 0x09:  // Class Code
            if (size >= 3) {
                value = (m_classcode[0]) |
                       (m_classcode[1] << 8) |
                       (m_classcode[2] << 16);
            }
            break;
            
        case 0x0E:  // Header Type
            if (size >= 1) {
                value = m_headerType;
            }
            break;
            
        case 0x10:  // BAR0 - I/O Space
            if (size >= 4) {
                value = m_bar0;
            }
            break;
            
        case 0x14:  // BAR1 - Memory Space
            if (size >= 4) {
                value = m_bar1;
            }
            break;
            
        case 0x3C:  // Interrupt Line
            if (size >= 1) {
                value = m_interruptLine;
            }
            break;
            
        case 0x3D:  // Interrupt Pin
            if (size >= 1) {
                value = m_interruptPin;
            }
            break;
    }
    
    return value;
}

void RTL8139PCI::WriteConfig(uint8_t reg, uint32_t value, int size)
{
    switch (reg) {
        case 0x04:  // Command
            if (size >= 2) {
                m_command = static_cast<uint16_t>(value);
                
                // Check if the I/O or memory space has been enabled/disabled
                bool ioEnabled = (m_command & 0x0001) != 0;
                bool memEnabled = (m_command & 0x0002) != 0;
                
                // Handle changes to the command register
                // This would affect the behavior of the device
            }
            break;
            
        case 0x10:  // BAR0 - I/O Space
            if (size >= 4) {
                // Handle BAR0 write
                if (value == 0xFFFFFFFF) {
                    // Size request - return the size of the I/O space
                    m_bar0 = ~(m_ioSize - 1) | 0x01;  // I/O space, 256 bytes
                } else {
                    // Address assignment
                    m_bar0 = (value & ~0x03) | 0x01;  // I/O space, preserve the I/O bit
                    m_ioBase = static_cast<uint16_t>(m_bar0 & ~0x03);
                }
            }
            break;
            
        case 0x14:  // BAR1 - Memory Space
            if (size >= 4) {
                // Handle BAR1 write
                if (value == 0xFFFFFFFF) {
                    // Size request - return the size of the memory space
                    m_bar1 = ~(m_memSize - 1) & 0xFFFFFFF0;  // Memory space, 256 bytes, 32-bit
                } else {
                    // Address assignment
                    m_bar1 = value & 0xFFFFFFF0;  // Memory space, 32-bit
                    m_memBase = m_bar1;
                }
            }
            break;
            
        case 0x3C:  // Interrupt Line
            if (size >= 1) {
                m_interruptLine = static_cast<uint8_t>(value);
                
                // Update the IRQ in the RTL8139
                m_adapter->SetIRQ(m_interruptLine);
            }
            break;
    }
}

uint32_t RTL8139PCI::ReadMemory(uint32_t address, int size)
{
    // Check if memory space is enabled
    if (!(m_command & 0x0002)) {
        return 0xFFFFFFFF;
    }
    
    // Check if the address is within our memory range
    if (address < m_memBase || address >= m_memBase + m_memSize) {
        return 0xFFFFFFFF;
    }
    
    // Calculate the register address
    uint16_t regAddr = static_cast<uint16_t>(address - m_memBase);
    
    // Forward to the RTL8139
    return m_adapter->ReadRegister(regAddr, size);
}

void RTL8139PCI::WriteMemory(uint32_t address, uint32_t value, int size)
{
    // Check if memory space is enabled
    if (!(m_command & 0x0002)) {
        return;
    }
    
    // Check if the address is within our memory range
    if (address < m_memBase || address >= m_memBase + m_memSize) {
        return;
    }
    
    // Calculate the register address
    uint16_t regAddr = static_cast<uint16_t>(address - m_memBase);
    
    // Forward to the RTL8139
    m_adapter->WriteRegister(regAddr, value, size);
}

uint32_t RTL8139PCI::ReadIO(uint16_t address, int size)
{
    // Check if I/O space is enabled
    if (!(m_command & 0x0001)) {
        return 0xFFFFFFFF;
    }
    
    // Check if the address is within our I/O range
    if (address < m_ioBase || address >= m_ioBase + m_ioSize) {
        return 0xFFFFFFFF;
    }
    
    // Calculate the register address
    uint16_t regAddr = static_cast<uint16_t>(address - m_ioBase);
    
    // Forward to the RTL8139
    return m_adapter->ReadRegister(regAddr, size);
}

void RTL8139PCI::WriteIO(uint16_t address, uint32_t value, int size)
{
    // Check if I/O space is enabled
    if (!(m_command & 0x0001)) {
        return;
    }
    
    // Check if the address is within our I/O range
    if (address < m_ioBase || address >= m_ioBase + m_ioSize) {
        return;
    }
    
    // Calculate the register address
    uint16_t regAddr = static_cast<uint16_t>(address - m_ioBase);
    
    // Forward to the RTL8139
    m_adapter->WriteRegister(regAddr, value, size);
}

void RTL8139PCI::SetIRQ(int irq)
{
    m_interruptLine = static_cast<uint8_t>(irq);
    
    // Update the RTL8139
    m_adapter->SetIRQ(irq);
}

RTL8139* RTL8139PCI::GetAdapter() const
{
    return m_adapter.get();
}

} // namespace x86emu