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

NFORCE_IDE::NFORCE_IDE(const DeviceConfig& config)
    : PCIDevice(config),
      m_isaBridge(nullptr),
      m_classRev(0),
      m_priCompatMode(true),
      m_secCompatMode(true)
{
}

bool NFORCE_IDE::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize as a PCI device
    if (!PCIDevice::Initialize()) {
        return false;
    }
    
    // Set multifunction bit in header type
    m_pciConfig[PCI_REG_HEADER_TYPE / 4] |= 0x80;
    
    // Initially set compatibility mode for both channels
    m_priCompatMode = true;
    m_secCompatMode = true;
    
    // Setup IO BARs for IDE controller
    // BAR0: Primary command registers (typically 0x1F0-0x1F7)
    m_pciConfig[PCI_REG_BAR0 / 4] = 0x1F01;  // I/O space, base will be assigned by BIOS/OS
    
    // BAR1: Primary control registers (typically 0x3F6-0x3F7)
    m_pciConfig[PCI_REG_BAR1 / 4] = 0x3F61;  // I/O space, base will be assigned by BIOS/OS
    
    // BAR2: Secondary command registers (typically 0x170-0x177)
    m_pciConfig[PCI_REG_BAR2 / 4] = 0x1701;  // I/O space, base will be assigned by BIOS/OS
    
    // BAR3: Secondary control registers (typically 0x376-0x377)
    m_pciConfig[PCI_REG_BAR3 / 4] = 0x3761;  // I/O space, base will be assigned by BIOS/OS
    
    // BAR4: Bus mastering registers (typically 0xFF60)
    m_pciConfig[PCI_REG_BAR4 / 4] = 0xFF61;  // I/O space, base will be assigned by BIOS/OS
    
    // Set up IDE class/revision
    m_classRev = 0x01018A00 | 0xC3;  // IDE controller with native PCI mode support, revision C3
    m_pciConfig[PCI_REG_CLASS_REVISION / 4] = m_classRev;
    
    // bit 0 of class byte 0 specifies primary channel mode (0=compat, 1=native PCI)
    // bit 2 of class byte 0 specifies secondary channel mode (0=compat, 1=native PCI)
    m_priCompatMode = !(m_classRev & 0x01000000);
    m_secCompatMode = !(m_classRev & 0x04000000);
    
    // Set initial status
    m_pciConfig[PCI_REG_STATUS / 4] = 0x0210;  // Medium timing, supports back-to-back transactions
    
    // Set command
    m_pciConfig[PCI_REG_COMMAND / 4] = 0x0001;  // I/O space access enabled
    
    // Set interrupt pin
    m_pciConfig[PCI_REG_INTERRUPT_PIN / 4] = 0x01;  // INTA#
    
    SetInitialized(true);
    return true;
}

void NFORCE_IDE::Reset() {
    PCIDevice::Reset();
    
    // Reset configuration
    m_pciConfig[PCI_REG_CLASS_REVISION / 4] = m_classRev;
    m_priCompatMode = !(m_classRev & 0x01000000);
    m_secCompatMode = !(m_classRev & 0x04000000);
}

uint32_t NFORCE_IDE::ReadConfig(uint8_t reg, int size) {
    return PCIDevice::ReadConfig(reg, size);
}

void NFORCE_IDE::WriteConfig(uint8_t reg, uint32_t value, int size) {
    // Special handling for class/revision
    if (reg == PCI_REG_CLASS_REVISION && (size >= 2)) {
        // Only allow changing the IDE compatibility mode bits
        uint32_t oldValue = m_pciConfig[PCI_REG_CLASS_REVISION / 4];
        uint32_t newMode = (value & 0x05000000);  // Extract mode bits
        uint32_t newValue = (oldValue & 0xFAFFFFFF) | newMode;  // Apply only mode bits
        
        m_pciConfig[PCI_REG_CLASS_REVISION / 4] = newValue;
        m_priCompatMode = !(newValue & 0x01000000);
        m_secCompatMode = !(newValue & 0x04000000);
        
        LOG_DEBUG("nForce IDE: Mode changed - Primary: %s, Secondary: %s\n",
                 m_priCompatMode ? "Compatibility" : "Native PCI",
                 m_secCompatMode ? "Compatibility" : "Native PCI");
                 
        return;
    }
    
    PCIDevice::WriteConfig(reg, value, size);
}

uint32_t NFORCE_IDE::ReadIO(uint16_t port, int size) {
    uint32_t value = 0;
    
    // Check if in compatibility mode or using BARs
    if (m_priCompatMode && (port >= 0x1F0 && port <= 0x1F7)) {
        return ReadPrimaryCommand(port - 0x1F0);
    }
    
    if (m_priCompatMode && (port >= 0x3F6 && port <= 0x3F7)) {
        return ReadPrimaryControl(port - 0x3F6);
    }
    
    if (m_secCompatMode && (port >= 0x170 && port <= 0x177)) {
        return ReadSecondaryCommand(port - 0x170);
    }
    
    if (m_secCompatMode && (port >= 0x376 && port <= 0x377)) {
        return ReadSecondaryControl(port - 0x376);
    }
    
    // Check BAR accesses
    uint16_t pri_cmd_base = m_pciConfig[PCI_REG_BAR0 / 4] & 0xFFF8;  // Mask off I/O space bit
    uint16_t pri_ctl_base = m_pciConfig[PCI_REG_BAR1 / 4] & 0xFFF8;
    uint16_t sec_cmd_base = m_pciConfig[PCI_REG_BAR2 / 4] & 0xFFF8;
    uint16_t sec_ctl_base = m_pciConfig[PCI_REG_BAR3 / 4] & 0xFFF8;
    uint16_t bm_base = m_pciConfig[PCI_REG_BAR4 / 4] & 0xFFF8;
    
    if ((port >= pri_cmd_base) && (port < pri_cmd_base + 8) && !m_priCompatMode) {
        return ReadPrimaryCommand(port - pri_cmd_base);
    }
    
    if ((port >= pri_ctl_base) && (port < pri_ctl_base + 4) && !m_priCompatMode) {
        return ReadPrimaryControl(port - pri_ctl_base);
    }
    
    if ((port >= sec_cmd_base) && (port < sec_cmd_base + 8) && !m_secCompatMode) {
        return ReadSecondaryCommand(port - sec_cmd_base);
    }
    
    if ((port >= sec_ctl_base) && (port < sec_ctl_base + 4) && !m_secCompatMode) {
        return ReadSecondaryControl(port - sec_ctl_base);
    }
    
    if ((port >= bm_base) && (port < bm_base + 16)) {
        return ReadBusMaster(port - bm_base);
    }
    
    LOG_DEBUG("nForce IDE: Read from unknown port %04X\n", port);
    return 0xFF;
}

void NFORCE_IDE::WriteIO(uint16_t port, uint32_t value, int size) {
    // Check if in compatibility mode or using BARs
    if (m_priCompatMode && (port >= 0x1F0 && port <= 0x1F7)) {
        WritePrimaryCommand(port - 0x1F0, value & 0xFF);
        return;
    }
    
    if (m_priCompatMode && (port >= 0x3F6 && port <= 0x3F7)) {
        WritePrimaryControl(port - 0x3F6, value & 0xFF);
        return;
    }
    
    if (m_secCompatMode && (port >= 0x170 && port <= 0x177)) {
        WriteSecondaryCommand(port - 0x170, value & 0xFF);
        return;
    }
    
    if (m_secCompatMode && (port >= 0x376 && port <= 0x377)) {
        WriteSecondaryControl(port - 0x376, value & 0xFF);
        return;
    }
    
    // Check BAR accesses
    uint16_t pri_cmd_base = m_pciConfig[PCI_REG_BAR0 / 4] & 0xFFF8;  // Mask off I/O space bit
    uint16_t pri_ctl_base = m_pciConfig[PCI_REG_BAR1 / 4] & 0xFFF8;
    uint16_t sec_cmd_base = m_pciConfig[PCI_REG_BAR2 / 4] & 0xFFF8;
    uint16_t sec_ctl_base = m_pciConfig[PCI_REG_BAR3 / 4] & 0xFFF8;
    uint16_t bm_base = m_pciConfig[PCI_REG_BAR4 / 4] & 0xFFF8;
    
    if ((port >= pri_cmd_base) && (port < pri_cmd_base + 8) && !m_priCompatMode) {
        WritePrimaryCommand(port - pri_cmd_base, value & 0xFF);
        return;
    }
    
    if ((port >= pri_ctl_base) && (port < pri_ctl_base + 4) && !m_priCompatMode) {
        WritePrimaryControl(port - pri_ctl_base, value & 0xFF);
        return;
    }
    
    if ((port >= sec_cmd_base) && (port < sec_cmd_base + 8) && !m_secCompatMode) {
        WriteSecondaryCommand(port - sec_cmd_base, value & 0xFF);
        return;
    }
    
    if ((port >= sec_ctl_base) && (port < sec_ctl_base + 4) && !m_secCompatMode) {
        WriteSecondaryControl(port - sec_ctl_base, value & 0xFF);
        return;
    }
    
    if ((port >= bm_base) && (port < bm_base + 16)) {
        WriteBusMaster(port - bm_base, value);
        return;
    }
    
    LOG_DEBUG("nForce IDE: Write to unknown port %04X = %02X\n", port, value & 0xFF);
}

uint8_t NFORCE_IDE::ReadPrimaryCommand(uint16_t port) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Read from primary command port %02X\n", port);
    return 0xFF;
}

void NFORCE_IDE::WritePrimaryCommand(uint16_t port, uint8_t value) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Write to primary command port %02X = %02X\n", port, value);
}

uint8_t NFORCE_IDE::ReadPrimaryControl(uint16_t port) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Read from primary control port %02X\n", port);
    return 0xFF;
}

void NFORCE_IDE::WritePrimaryControl(uint16_t port, uint8_t value) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Write to primary control port %02X = %02X\n", port, value);
    
    // Check for reset bit (bit 2)
    if (port == 2) {
        if (value & 0x04) {
            LOG_DEBUG("nForce IDE: Primary channel reset asserted\n");
        } else {
            LOG_DEBUG("nForce IDE: Primary channel reset deasserted\n");
        }
    }
}

uint8_t NFORCE_IDE::ReadSecondaryCommand(uint16_t port) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Read from secondary command port %02X\n", port);
    return 0xFF;
}

void NFORCE_IDE::WriteSecondaryCommand(uint16_t port, uint8_t value) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Write to secondary command port %02X = %02X\n", port, value);
}

uint8_t NFORCE_IDE::ReadSecondaryControl(uint16_t port) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Read from secondary control port %02X\n", port);
    return 0xFF;
}

void NFORCE_IDE::WriteSecondaryControl(uint16_t port, uint8_t value) {
    // TODO: Implement actual IDE controller
    LOG_DEBUG("nForce IDE: Write to secondary control port %02X = %02X\n", port, value);
    
    // Check for reset bit (bit 2)
    if (port == 2) {
        if (value & 0x04) {
            LOG_DEBUG("nForce IDE: Secondary channel reset asserted\n");
        } else {
            LOG_DEBUG("nForce IDE: Secondary channel reset deasserted\n");
        }
    }
}

uint32_t NFORCE_IDE::ReadBusMaster(uint16_t port) {
    // TODO: Implement bus mastering
    LOG_DEBUG("nForce IDE: Read from bus master port %02X\n", port);
    return 0xFF;
}

void NFORCE_IDE::WriteBusMaster(uint16_t port, uint32_t value) {
    // TODO: Implement bus mastering
    LOG_DEBUG("nForce IDE: Write to bus master port %02X = %08X\n", port, value);
}