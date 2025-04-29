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

NFORCE_AGPBridge::NFORCE_AGPBridge(const DeviceConfig& config)
    : PCIDevice(config)
{
}

bool NFORCE_AGPBridge::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize as a PCI device
    if (!PCIDevice::Initialize()) {
        return false;
    }
    
    // Set multifunction bit in header type (optional for bridge)
    m_pciConfig[PCI_REG_HEADER_TYPE / 4] |= 0x01;  // Type 1 PCI-to-PCI bridge
    
    // Set primary/secondary bus numbers
    // Note: These will be set by the BIOS/OS during enumeration
    m_pciConfig[PCI_REG_PRIMARY_BUS / 4] = 0x0001001E;  // Primary=0x1E, Secondary=0x01, Subordinate=0x01
    
    // Set up I/O and memory windows (configured by BIOS/OS)
    m_pciConfig[PCI_REG_IO_BASE / 4] = 0x000000F0;  // I/O base = 0xF000, limit = 0xFFFF
    m_pciConfig[PCI_REG_MEMORY_BASE / 4] = 0xFEE0FED0;  // Memory base = 0xFED00000, limit = 0xFEDFFFFF
    m_pciConfig[PCI_REG_PREFETCH_BASE / 4] = 0xF0000000;  // Prefetch base = 0x00000000, limit = 0xEFFFFFFF
    m_pciConfig[PCI_REG_PREFETCH_BASE_UPPER / 4] = 0x00000000;
    m_pciConfig[PCI_REG_PREFETCH_LIMIT_UPPER / 4] = 0x00000000;
    
    // Set status
    m_pciConfig[PCI_REG_STATUS / 4] = 0x0020;  // 66MHz capable
    
    // Set command
    m_pciConfig[PCI_REG_COMMAND / 4] = 0x0003;  // I/O space and memory space access enabled
    
    // Set bridge control
    m_pciConfig[PCI_REG_BRIDGE_CONTROL / 4] = 0x0000;  // No VGA, no error reporting, etc.
    
    // AGP specific registers
    for (uint8_t reg = 0x40; reg < 0x80; reg += 4) {
        m_pciConfig[reg / 4] = 0;
    }
    
    // AGP Status register (0x44)
    m_pciConfig[0x44 / 4] = 0x1F000114;
    
    // AGP Command register (0x48)
    m_pciConfig[0x48 / 4] = 0x00000017;
    
    // Various unknown AGP registers used by nForce
    m_pciConfig[0x4C / 4] = 0x00000001;
    m_pciConfig[0x54 / 4] = 0x00000002;
    
    SetInitialized(true);
    return true;
}

void NFORCE_AGPBridge::Reset() {
    PCIDevice::Reset();
    
    // Reset AGP-specific registers while keeping device config
    m_pciConfig[0x44 / 4] = 0x1F000114;
    m_pciConfig[0x48 / 4] = 0x00000017;
    m_pciConfig[0x4C / 4] = 0x00000001;
    m_pciConfig[0x54 / 4] = 0x00000002;
    
    // Reset bus numbers - BIOS will set these during initialization
    m_pciConfig[PCI_REG_PRIMARY_BUS / 4] = 0x0001001E;
    
    // Reset PCI windows - BIOS will configure these
    m_pciConfig[PCI_REG_IO_BASE / 4] = 0x000000F0;
    m_pciConfig[PCI_REG_MEMORY_BASE / 4] = 0xFEE0FED0;
    m_pciConfig[PCI_REG_PREFETCH_BASE / 4] = 0xF0000000;
    m_pciConfig[PCI_REG_PREFETCH_BASE_UPPER / 4] = 0x00000000;
    m_pciConfig[PCI_REG_PREFETCH_LIMIT_UPPER / 4] = 0x00000000;
}

uint32_t NFORCE_AGPBridge::ReadConfig(uint8_t reg, int size) {
    if (reg >= 0x40 && reg < 0x80) {
        // AGP configuration registers
        return UnknownRead(reg - 0x40);
    }
    
    return PCIDevice::ReadConfig(reg, size);
}

void NFORCE_AGPBridge::WriteConfig(uint8_t reg, uint32_t value, int size) {
    if (reg >= 0x40 && reg < 0x80) {
        // AGP configuration registers
        UnknownWrite(reg - 0x40, value);
        return;
    }
    
    PCIDevice::WriteConfig(reg, value, size);
}

uint32_t NFORCE_AGPBridge::UnknownRead(uint32_t offset) {
    uint32_t reg = 0x40 + offset;
    uint32_t value = m_pciConfig[reg / 4];
    
    // Special handling for register 0x4C
    if (reg == 0x4C) {
        value |= 0x00000001;  // Set bit 0 - needed by some AGP drivers
    }
    
    LOG_DEBUG("nForce AGP Bridge: Read from AGP register %02X = %08X\n", reg, value);
    return value;
}

void NFORCE_AGPBridge::UnknownWrite(uint32_t offset, uint32_t data) {
    uint32_t reg = 0x40 + offset;
    
    LOG_DEBUG("nForce AGP Bridge: Write to AGP register %02X = %08X\n", reg, data);
    m_pciConfig[reg / 4] = data;
}