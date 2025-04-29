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

NFORCE_CpuBridge::NFORCE_CpuBridge(const DeviceConfig& config)
    : PCIDevice(config),
      m_ramSize(0),
      m_biosrom(nullptr),
      m_ramSizeReg(0)
{
}

bool NFORCE_CpuBridge::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize as a PCI device
    if (!PCIDevice::Initialize()) {
        return false;
    }
    
    // Default to 128MB RAM
    m_ramSize = 128;
    
    // Set initial status
    m_pciConfig[PCI_REG_STATUS / 4] = 0x00b0;
    
    // Set multi-function bit in header type
    m_pciConfig[PCI_REG_HEADER_TYPE / 4] |= 0x80;
    
    SetInitialized(true);
    return true;
}

void NFORCE_CpuBridge::Reset() {
    PCIDevice::Reset();
    ResetAllMappings();
}

uint32_t NFORCE_CpuBridge::ReadConfig(uint8_t reg, int size) {
    uint32_t value = 0;
    
    if (reg >= 0x10 && reg < 0x28) {
        // Base address registers
        return PCIDevice::ReadConfig(reg, size);
    }
    else if (reg == 0x84) {
        // RAM size register
        return m_ramSizeReg;
    }
    else if (reg == 0xF0) {
        // Unknown register used by nForce
        return 4;  // This value is observed in MAME
    }
    
    return PCIDevice::ReadConfig(reg, size);
}

void NFORCE_CpuBridge::WriteConfig(uint8_t reg, uint32_t value, int size) {
    if (reg >= 0x10 && reg < 0x28) {
        // Base address registers
        PCIDevice::WriteConfig(reg, value, size);
        // TODO: Handle memory map changes
    }
    else if (reg == 0x84) {
        // RAM size register
        LOG_DEBUG("nForce CPU Bridge: Setting RAM size register to %08X\n", value);
        m_ramSizeReg = value;
    }
    else if (reg == 0xF0) {
        // Unknown register used by nForce
        LOG_DEBUG("nForce CPU Bridge: Writing %08X to unknown register F0\n", value);
    }
    else {
        PCIDevice::WriteConfig(reg, value, size);
    }
}

void NFORCE_CpuBridge::ResetAllMappings() {
    // Reset any memory mappings we have
}

void NFORCE_CpuBridge::SetRAMSize(uint32_t size) {
    m_ramSize = size;
    m_ramSizeReg = (size * 1024 * 1024) - 1;  // As used by nForce BIOS
    LOG_INFO("nForce CPU Bridge: RAM size set to %d MB\n", size);
}

void NFORCE_CpuBridge::MapMemoryWindows() {
    // TODO: Implement if needed
}