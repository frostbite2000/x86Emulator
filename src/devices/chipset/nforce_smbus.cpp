/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2024 frostbite2000
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

#include "devices/chipset/nforce/nforce.h"
#include "log.h"

NFORCE_SMBus::NFORCE_SMBus(const DeviceConfig& config)
    : PCIDevice(config),
      m_isaBridge(nullptr)
{
    // Initialize SMBus state
    for (int b = 0; b < 2; b++) {
        m_smbusState[b].status = 0;
        m_smbusState[b].control = 0;
        m_smbusState[b].address = 0;
        m_smbusState[b].data = 0;
        m_smbusState[b].command = 0;
        m_smbusState[b].rw = 0;
        
        for (int a = 0; a < 128; a++) {
            m_smbusState[b].devices[a] = nullptr;
        }
        
        memset(m_smbusState[b].words, 0, sizeof(m_smbusState[b].words));
    }
}

bool NFORCE_SMBus::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize as a PCI device
    if (!PCIDevice::Initialize()) {
        return false;
    }
    
    // Set up IO space for SMBus controller
    // IO BAR0: SMBus 0 - typically at 0x1000
    m_pciConfig[PCI_REG_BAR0 / 4] = 0x1001;  // IO space, base will be assigned by BIOS/OS
    
    // IO BAR1: SMBus 1 - typically at 0xC000
    m_pciConfig[PCI_REG_BAR1 / 4] = 0xC001;  // IO space, base will be assigned by BIOS/OS
    
    // IO BAR2: Additional SMBus registers - typically at 0xC200
    m_pciConfig[PCI_REG_BAR2 / 4] = 0xC201;  // IO space, base will be assigned by BIOS/OS
    
    // Set status
    m_pciConfig[PCI_REG_STATUS / 4] = 0x00B0;
    
    // Set command
    m_pciConfig[PCI_REG_COMMAND / 4] = 0x0001;  // IO space access enabled
    
    // Set interrupt pin 
    m_pciConfig[PCI_REG_INTERRUPT_PIN / 4] = 0x01;  // INTA#
    
    // Set multifunction bit in header type
    m_pciConfig[PCI_REG_HEADER_TYPE / 4] |= 0x80;
    
    SetInitialized(true);
    return true;
}

void NFORCE_SMBus::Reset() {
    PCIDevice::Reset();
    
    // Reset SMBus state
    for (int b = 0; b < 2; b++) {
        m_smbusState[b].status = 0;
        m_smbusState[b].control = 0;
        m_smbusState[b].address = 0;
        m_smbusState[b].data = 0;
        m_smbusState[b].command = 0;
        m_smbusState[b].rw = 0;
        
        memset(m_smbusState[b].words, 0, sizeof(m_smbusState[b].words));
    }
}

uint32_t NFORCE_SMBus::ReadConfig(uint8_t reg, int size) {
    if (reg == 0x3e) {
        // Min Grant
        return 3;
    }
    else if (reg == 0x3f) {
        // Max Latency
        return 1;
    }
    
    return PCIDevice::ReadConfig(reg, size);
}

void NFORCE_SMBus::WriteConfig(uint8_t reg, uint32_t value, int size) {
    PCIDevice::WriteConfig(reg, value, size);
}

uint32_t NFORCE_SMBus::ReadIO(uint16_t port, int size) {
    // Determine which SMBus controller is being accessed
    int smbusIndex = 0;
    uint32_t offset = 0;
    
    uint16_t smbus0_base = m_pciConfig[PCI_REG_BAR0 / 4] & 0xFFF0;  // Mask off IO space bit
    uint16_t smbus1_base = m_pciConfig[PCI_REG_BAR1 / 4] & 0xFFF0;
    uint16_t smbus2_base = m_pciConfig[PCI_REG_BAR2 / 4] & 0xFFF0;
    
    if (port >= smbus0_base && port < smbus0_base + 0x10) {
        smbusIndex = 0;
        offset = port - smbus0_base;
    }
    else if (port >= smbus1_base && port < smbus1_base + 0x10) {
        smbusIndex = 1;
        offset = port - smbus1_base;
    }
    else if (port >= smbus2_base && port < smbus2_base + 0x20) {
        // This is additional SMBus registers, not implementing for now
        return 0;
    }
    else {
        LOG_ERROR("nForce SMBus: Read from unknown port %04X\n", port);
        return 0;
    }
    
    return SMBusRead(smbusIndex, offset, size_to_mask(size));
}

void NFORCE_SMBus::WriteIO(uint16_t port, uint32_t value, int size) {
    // Determine which SMBus controller is being accessed
    int smbusIndex = 0;
    uint32_t offset = 0;
    
    uint16_t smbus0_base = m_pciConfig[PCI_REG_BAR0 / 4] & 0xFFF0;  // Mask off IO space bit
    uint16_t smbus1_base = m_pciConfig[PCI_REG_BAR1 / 4] & 0xFFF0;
    uint16_t smbus2_base = m_pciConfig[PCI_REG_BAR2 / 4] & 0xFFF0;
    
    if (port >= smbus0_base && port < smbus0_base + 0x10) {
        smbusIndex = 0;
        offset = port - smbus0_base;
    }
    else if (port >= smbus1_base && port < smbus1_base + 0x10) {
        smbusIndex = 1;
        offset = port - smbus1_base;
    }
    else if (port >= smbus2_base && port < smbus2_base + 0x20) {
        // This is additional SMBus registers, not implementing for now
        return;
    }
    else {
        LOG_ERROR("nForce SMBus: Write to unknown port %04X = %08X\n", port, value);
        return;
    }
    
    SMBusWrite(smbusIndex, offset, value, size_to_mask(size));
}

uint32_t NFORCE_SMBus::SMBusRead(int bus, uint32_t offset, uint32_t mask) {
    // Register offsets:
    // 0x00 - SMBus status register (16 bits)
    // 0x02 - SMBus control register (8 bits)
    // 0x04 - SMBus address register (8 bits)
    // 0x06 - SMBus data register (16 bits)
    // 0x08 - SMBus command register (8 bits)
    
    uint32_t value = 0;
    int wordOffset = offset / 4;
    
    LOG_DEBUG("SMBus %d: Read from offset %02X, mask %08X\n", bus, offset, mask);
    
    if (offset == 0) { // Status register at offset 0
        m_smbusState[bus].words[0] = (m_smbusState[bus].words[0] & ~0xFFFF) | 
                                    ((m_smbusState[bus].status & 0xFFFF) << 0);
    }
    else if (offset == 6) { // Data register at offset 6
        m_smbusState[bus].words[1] = (m_smbusState[bus].words[1] & ~(0xFFFF << 16)) | 
                                    ((m_smbusState[bus].data & 0xFFFF) << 16);
    }
    
    value = m_smbusState[bus].words[wordOffset];
    return value;
}

void NFORCE_SMBus::SMBusWrite(int bus, uint32_t offset, uint32_t data, uint32_t mask) {
    // Register offsets:
    // 0x00 - SMBus status register (16 bits)
    // 0x02 - SMBus control register (8 bits)
    // 0x04 - SMBus address register (8 bits)
    // 0x06 - SMBus data register (16 bits)
    // 0x08 - SMBus command register (8 bits)
    
    int wordOffset = offset / 4;
    
    LOG_DEBUG("SMBus %d: Write to offset %02X, data %08X, mask %08X\n", bus, offset, data, mask);
    
    // Update the raw word
    uint32_t& word = m_smbusState[bus].words[wordOffset];
    word = (word & ~mask) | (data & mask);
    
    // Handle SMBus status register clear
    if ((offset == 0) && (mask & 0xFFFF)) {
        // Writing to status register clears bits
        m_smbusState[bus].status &= ~(data & 0xFFFF);
        
        // If we're clearing an interrupt, notify the ISA bridge
        if (data & 0x10) {
            if (m_isaBridge) {
                m_isaBridge->DeassertIRQ(10); // SMBus uses IRQ 10 typically
            }
        }
    }
    
    // Handle SMBus control register
    if ((offset == 2) && (mask & 0xFF0000)) {
        uint8_t controlValue = (data >> 16) & 0xFF;
        m_smbusState[bus].control = controlValue;
        
        // Check if the START bit is set (bit 3)
        if (controlValue & 0x08) {
            // Execute SMBus command
            int cycletype = controlValue & 7;
            
            // Only handle "byte data" commands (type 2)
            if ((cycletype & 6) == 2) {
                // Execute command on target device if it exists
                if (m_smbusState[bus].devices[m_smbusState[bus].address]) {
                    if (m_smbusState[bus].rw == 0) {
                        // Write operation
                        m_smbusState[bus].devices[m_smbusState[bus].address]->executeCommand(
                            m_smbusState[bus].command, 
                            m_smbusState[bus].rw, 
                            m_smbusState[bus].data);
                    } else {
                        // Read operation
                        m_smbusState[bus].data = m_smbusState[bus].devices[m_smbusState[bus].address]->executeCommand(
                            m_smbusState[bus].command, 
                            m_smbusState[bus].rw, 
                            m_smbusState[bus].data);
                    }
                } else {
                    LOG_DEBUG("SMBus %d: access to missing device at address %d\n", bus, m_smbusState[bus].address);
                }
                
                // Set interrupt flag
                m_smbusState[bus].status |= 0x10;
                
                // If interrupts are enabled (bit 4), trigger IRQ
                if (controlValue & 0x10) {
                    if (m_isaBridge) {
                        m_isaBridge->AssertIRQ(10); // SMBus uses IRQ 10 typically
                    }
                }
            }
        }
    }
    
    // Handle SMBus address register
    if ((offset == 4) && (mask & 0xFF)) {
        m_smbusState[bus].address = data >> 1;  // Address is in the upper 7 bits
        m_smbusState[bus].rw = data & 1;        // R/W bit is the lowest bit
    }
    
    // Handle SMBus data register
    if ((offset == 6) && (mask & 0xFF0000)) {
        m_smbusState[bus].data = (data >> 16) & 0xFF;
    }
    
    // Handle SMBus command register
    if ((offset == 8) && (mask & 0xFF)) {
        m_smbusState[bus].command = data & 0xFF;
    }
}

void NFORCE_SMBus::RegisterSMBusDevice(int bus, int address, SMBusDevice* device) {
    if (bus >= 0 && bus < 2 && address >= 0 && address < 128) {
        m_smbusState[bus].devices[address] = device;
        LOG_DEBUG("SMBus %d: Registered device at address %02X\n", bus, address);
    } else {
        LOG_ERROR("SMBus: Invalid bus or address for device registration: bus=%d, addr=%02X\n", bus, address);
    }
}