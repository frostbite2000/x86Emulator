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

NFORCE_Memory::NFORCE_Memory(const DeviceConfig& config)
    : PCIDevice(config),
      m_ddrRamSize(0),
      m_hostBridge(nullptr)
{
}

bool NFORCE_Memory::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize as a PCI device
    if (!PCIDevice::Initialize()) {
        return false;
    }
    
    // Set multi-function bit in header type
    m_pciConfig[PCI_REG_HEADER_TYPE / 4] |= 0x80;
    
    // Default RAM size to 256MB
    m_ddrRamSize = 256;
    
    // Initial status
    m_pciConfig[PCI_REG_STATUS / 4] = 0x0020;
    
    SetInitialized(true);
    return true;
}

void NFORCE_Memory::Reset() {
    PCIDevice::Reset();
    
    // Notify the host bridge about our RAM size
    if (m_hostBridge) {
        m_hostBridge->SetRAMSize(m_ddrRamSize);
    }
}

uint32_t NFORCE_Memory::ReadConfig(uint8_t reg, int size) {
    if (reg >= 0xA0 && reg <= 0xB7) {
        // Special memory controller registers
        // These must have bit 31 as 0
        if (size == 4) {
            return 0x7FFFFFFF;  // Return with high bit clear
        }
    }
    else if (reg >= 0xC4 && reg <= 0xCB) {
        // Special memory controller registers
        // These must have bit 0 as 0
        if (size == 4) {
            return 0xFFFFFFFE;  // Return with low bit clear
        }
    }
    
    return PCIDevice::ReadConfig(reg, size);
}

void NFORCE_Memory::WriteConfig(uint8_t reg, uint32_t value, int size) {
    if (reg >= 0xA0 && reg <= 0xB7) {
        // Special memory controller registers
        // These must have bit 31 as 0, force it
        if (size == 4) {
            value &= 0x7FFFFFFF;
        }
    }
    else if (reg >= 0xC4 && reg <= 0xCB) {
        // Special memory controller registers
        // These must have bit 0 as 0, force it
        if (size == 4) {
            value &= 0xFFFFFFFE;
        }
    }
    
    PCIDevice::WriteConfig(reg, value, size);
}

void NFORCE_Memory::SetupRAM(void* ramPtr, uint32_t ramSize) {
    m_ddrRamSize = ramSize;
    
    // Calculate how many 32-bit entries we need
    size_t entries = (ramSize * 1024 * 1024) / 4;
    
    if (entries > 0) {
        // Resize the RAM buffer
        m_ram.resize(entries);
        
        // Copy the RAM if we have a source pointer
        if (ramPtr) {
            memcpy(&m_ram[0], ramPtr, entries * 4);
        }
        
        LOG_INFO("nForce Memory Controller: RAM buffer initialized with %zu MB\n", ramSize);
    }
    
    // Update the CPU bridge with the RAM size
    if (m_hostBridge) {
        m_hostBridge->SetRAMSize(ramSize);
    }
}