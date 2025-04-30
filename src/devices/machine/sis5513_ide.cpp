/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Portions Copyright (C) 2011-2025 MAME development team (original SiS 5513 implementation)
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

#include "devices/machine/sis5513_ide.h"
#include "logger.h"

namespace x86emu {

SiS5513IdeDevice::SiS5513IdeDevice()
    : PCIDevice(0x10395513, 0xd0, 0x010180, 0x00)
    , m_ideCtrl0(0)
    , m_ideMisc(0)
{
    // Default BAR values for legacy mode
    m_bar[0] = 0x1f0;  // Primary command
    m_bar[1] = 0x3f4;  // Primary control
    m_bar[2] = 0x170;  // Secondary command
    m_bar[3] = 0x374;  // Secondary control
    m_bar[4] = 0xf00;  // Bus Master IDE Control
}

SiS5513IdeDevice::~SiS5513IdeDevice()
{
}

bool SiS5513IdeDevice::initialize()
{
    // Create IDE controllers
    m_ide1 = std::make_unique<IdeController>();
    m_ide2 = std::make_unique<IdeController>();
    
    // Initialize IDE controllers
    if (!m_ide1->initialize()) {
        Logger::GetInstance()->error("Failed to initialize primary IDE controller");
        return false;
    }
    
    if (!m_ide2->initialize()) {
        Logger::GetInstance()->error("Failed to initialize secondary IDE controller");
        return false;
    }
    
    // Set up IRQ callbacks
    m_ide1->setIRQCallback([this](bool state) {
        if (m_irqPrimaryCallback) {
            m_irqPrimaryCallback(state);
        }
    });
    
    m_ide2->setIRQCallback([this](bool state) {
        if (m_irqSecondaryCallback) {
            m_irqSecondaryCallback(state);
        }
    });
    
    return PCIDevice::initialize();
}

void SiS5513IdeDevice::reset()
{
    PCIDevice::reset();
    
    // Reset PCI configuration
    setCommand(0x0000);
    setCommandMask(5);
    setStatus(0x0000);
    setPCIClass(0x01018a);
    
    // Reset SiS-specific registers
    m_ideCtrl0 = 0;
    m_ideMisc = 0;
    
    // Reset BAR values
    m_bar[0] = 0x1f0;  // Primary command
    m_bar[1] = 0x3f4;  // Primary control
    m_bar[2] = 0x170;  // Secondary command
    m_bar[3] = 0x374;  // Secondary control
    m_bar[4] = 0xf00;  // Bus Master IDE Control
    
    // Reset the controllers
    if (m_ide1) m_ide1->reset();
    if (m_ide2) m_ide2->reset();
    
    // Update the BAR mappings
    flushIdeMode();
}

uint32_t SiS5513IdeDevice::configRead(int function, int reg, uint32_t mem_mask)
{
    // Handle function 0 only
    if (function != 0) {
        return 0xFFFFFFFF;
    }
    
    // Override header type register (0x0e)
    if (reg == 0x0e) {
        return 0x80;  // Needed for actual IDE detection
    }
    
    // Handle BAR registers
    if (reg >= 0x10 && reg <= 0x23) {
        int offset = (reg - 0x10) / 4;
        return barRead(offset);
    }
    
    // Handle SiS-specific registers
    switch (reg) {
        case 0x4a:
            return ideCtrl0Read();
            
        case 0x52:
            return ideMiscCtrlRead();
            
        default:
            return PCIDevice::configRead(function, reg, mem_mask);
    }
}

void SiS5513IdeDevice::configWrite(int function, int reg, uint32_t data, uint32_t mem_mask)
{
    // Handle function 0 only
    if (function != 0) {
        return;
    }
    
    // Handle programming interface register (0x09)
    if (reg == 0x09) {
        progIfWrite(data & 0xFF);
        return;
    }
    
    // Handle BAR registers
    if (reg >= 0x10 && reg <= 0x23) {
        int offset = (reg - 0x10) / 4;
        barWrite(offset, data);
        return;
    }
    
    // Handle SiS-specific registers
    switch (reg) {
        case 0x4a:
            ideCtrl0Write(data & 0xFF);
            break;
            
        case 0x52:
            ideMiscCtrlWrite(data & 0xFF);
            break;
            
        default:
            PCIDevice::configWrite(function, reg, data, mem_mask);
            break;
    }
}

void SiS5513IdeDevice::mapSpecialRegions(MemoryManager* memory_manager, IOManager* io_manager)
{
    // Map IDE I/O ports based on current configuration
    uint16_t primary_cmd = ide1Mode() ? m_bar[0] : 0x1f0;
    uint16_t primary_ctrl = ide1Mode() ? m_bar[1] : 0x3f4;
    uint16_t secondary_cmd = ide2Mode() ? m_bar[2] : 0x170;
    uint16_t secondary_ctrl = ide2Mode() ? m_bar[3] : 0x374;
    uint16_t bm_base = m_bar[4];
    
    // Map primary IDE controller ports
    io_manager->registerIOPortRange(
        primary_cmd, primary_cmd + 7,
        "IDE1_CMD",
        [this](uint16_t port) -> uint8_t {
            return ide1ReadCs0(port - m_bar[0]);
        },
        [this](uint16_t port, uint8_t value) {
            ide1WriteCs0(port - m_bar[0], value);
        }
    );
    
    io_manager->registerIOPortRange(
        primary_ctrl, primary_ctrl + 3,
        "IDE1_CTRL",
        [this](uint16_t port) -> uint8_t {
            if (port == m_bar[1] + 2) {
                return ide1ReadCs1();
            }
            return 0xFF;
        },
        [this](uint16_t port, uint8_t value) {
            if (port == m_bar[1] + 2) {
                ide1WriteCs1(value);
            }
        }
    );
    
    // Map secondary IDE controller ports
    io_manager->registerIOPortRange(
        secondary_cmd, secondary_cmd + 7,
        "IDE2_CMD",
        [this](uint16_t port) -> uint8_t {
            return ide2ReadCs0(port - m_bar[2]);
        },
        [this](uint16_t port, uint8_t value) {
            ide2WriteCs0(port - m_bar[2], value);
        }
    );
    
    io_manager->registerIOPortRange(
        secondary_ctrl, secondary_ctrl + 3,
        "IDE2_CTRL",
        [this](uint16_t port) -> uint8_t {
            if (port == m_bar[3] + 2) {
                return ide2ReadCs1();
            }
            return 0xFF;
        },
        [this](uint16_t port, uint8_t value) {
            if (port == m_bar[3] + 2) {
                ide2WriteCs1(value);
            }
        }
    );
    
    // Map Bus Master IDE Control ports
    // Primary channel
    io_manager->registerIOPortRange(
        bm_base, bm_base + 7,
        "IDE1_BM",
        [this](uint16_t port) -> uint8_t {
            return m_ide1->readBusMaster(port - m_bar[4]);
        },
        [this](uint16_t port, uint8_t value) {
            m_ide1->writeBusMaster(port - m_bar[4], value);
        }
    );
    
    // Secondary channel
    io_manager->registerIOPortRange(
        bm_base + 8, bm_base + 15,
        "IDE2_BM",
        [this](uint16_t port) -> uint8_t {
            return m_ide2->readBusMaster(port - (m_bar[4] + 8));
        },
        [this](uint16_t port, uint8_t value) {
            m_ide2->writeBusMaster(port - (m_bar[4] + 8), value);
        }
    );
}

void SiS5513IdeDevice::setIRQPrimaryCallback(std::function<void(bool)> callback)
{
    m_irqPrimaryCallback = callback;
}

void SiS5513IdeDevice::setIRQSecondaryCallback(std::function<void(bool)> callback)
{
    m_irqSecondaryCallback = callback;
}

IdeController* SiS5513IdeDevice::getPrimaryController()
{
    return m_ide1.get();
}

IdeController* SiS5513IdeDevice::getSecondaryController()
{
    return m_ide2.get();
}

inline bool SiS5513IdeDevice::ide1Mode()
{
    return (getPCIClass() & 0x3) == 3;
}

inline bool SiS5513IdeDevice::ide2Mode()
{
    return (getPCIClass() & 0xc) == 0xc;
}

void SiS5513IdeDevice::flushIdeMode()
{
    // This method is used to update BAR mapping based on IDE mode (PCI or legacy)
    // In compatible mode BARs with legacy addresses but can values written can still be readout.
    // In practice we need to override writes and make sure we flush remapping accordingly
    
    Logger::GetInstance()->debug("Updating IDE mode mapping (class=%08x)", getPCIClass());
    
    // Map Primary IDE Channel
    if (ide1Mode()) {
        // PCI Mode
        Logger::GetInstance()->debug("  Primary channel in PCI mode: BAR0=%04x BAR1=%04x", m_bar[0], m_bar[1]);
    } else {
        // Legacy Mode
        Logger::GetInstance()->debug("  Primary channel in legacy mode: 01F0/03F4");
        m_bar[0] = 0x1f0;
        m_bar[1] = 0x3f4;
    }

    // Map Secondary IDE Channel
    if (ide2Mode()) {
        // PCI Mode
        Logger::GetInstance()->debug("  Secondary channel in PCI mode: BAR2=%04x BAR3=%04x", m_bar[2], m_bar[3]);
    } else {
        // Legacy Mode
        Logger::GetInstance()->debug("  Secondary channel in legacy mode: 0170/0374");
        m_bar[2] = 0x170;
        m_bar[3] = 0x374;
    }
}

uint32_t SiS5513IdeDevice::barRead(int offset)
{
    if (offset >= 0 && offset < 5) {
        // Calculate BAR flags
        uint32_t flags = 0;
        
        // The first 4 BARs are I/O ports (primary cmd, primary ctrl, secondary cmd, secondary ctrl)
        if (offset < 4) {
            flags = 1; // I/O space indicator
        }
        
        // Return the BAR value
        return (m_bar[offset] & ~0xF) | flags;
    }
    
    return 0;
}

void SiS5513IdeDevice::barWrite(int offset, uint32_t data)
{
    if (offset >= 0 && offset < 5) {
        m_bar[offset] = data;
        Logger::GetInstance()->debug("IDE BAR[%d] set to %08x", offset, data);
        
        // Bits 0 (primary) and 2 (secondary) control if the mapping is legacy or BAR
        switch (offset) {
            case 0:
            case 1:
                // Primary channel
                if (!ide1Mode()) {
                    // Legacy mode - force standard values
                    m_bar[0] = 0x1f0;
                    m_bar[1] = 0x3f4;
                }
                break;
                
            case 2:
            case 3:
                // Secondary channel
                if (!ide2Mode()) {
                    // Legacy mode - force standard values
                    m_bar[2] = 0x170;
                    m_bar[3] = 0x374;
                }
                break;
        }
    }
}

void SiS5513IdeDevice::progIfWrite(uint8_t data)
{
    uint32_t oldVal = getPCIClass();
    uint32_t newVal = (oldVal & ~0xFF) | data;
    setPCIClass(newVal);
    
    // Check for switch to/from compatibility (legacy) mode from/to pci mode
    if ((oldVal ^ newVal) & 0xF) {
        flushIdeMode();
    }
}

uint8_t SiS5513IdeDevice::ideCtrl0Read()
{
    Logger::GetInstance()->debug("IDE ctrl 0 read [$4a] %02x", m_ideCtrl0);
    return m_ideCtrl0;
}

void SiS5513IdeDevice::ideCtrl0Write(uint8_t data)
{
    Logger::GetInstance()->debug("IDE ctrl 0 write [$4a] %02x", data);
    m_ideCtrl0 = data;
    // TODO: bit 1 disables IDE ch. 0, bit 2 ch. 1
}

uint8_t SiS5513IdeDevice::ideMiscCtrlRead()
{
    Logger::GetInstance()->debug("IDE misc ctrl read [$52] %02x", m_ideMisc);
    return m_ideMisc;
}

void SiS5513IdeDevice::ideMiscCtrlWrite(uint8_t data)
{
    Logger::GetInstance()->debug("IDE misc ctrl write [$52] %02x", data);
    m_ideMisc = data;

    const bool compatible_mode = (data & (1 << 2)) != 0;
    setPCIClass(getPCIClass() & ~0x7a); // Clear mode bits
    
    if (compatible_mode) {
        // Compatible Mode
        setInterruptPin(0); // No INTA#
    } else {
        // Native Mode
        setPCIClass(getPCIClass() | 0xa); // Set native mode bits
        setInterruptPin(1); // INTA#
    }

    flushIdeMode();
}

// IDE I/O port handlers
uint32_t SiS5513IdeDevice::ide1ReadCs0(int offset)
{
    if (!(getCommand() & 1)) {
        return 0xFFFFFFFF; // I/O disabled
    }
    
    return m_ide1->readCommandPort(offset);
}

void SiS5513IdeDevice::ide1WriteCs0(int offset, uint32_t data)
{
    if (!(getCommand() & 1)) {
        return; // I/O disabled
    }
    
    m_ide1->writeCommandPort(offset, data);
}

uint8_t SiS5513IdeDevice::ide1ReadCs1()
{
    if (!(getCommand() & 1)) {
        return 0xFF; // I/O disabled
    }
    
    return m_ide1->readControlPort();
}

void SiS5513IdeDevice::ide1WriteCs1(uint8_t data)
{
    if (!(getCommand() & 1)) {
        return; // I/O disabled
    }
    
    m_ide1->writeControlPort(data);
}

uint32_t SiS5513IdeDevice::ide2ReadCs0(int offset)
{
    if (!(getCommand() & 1)) {
        return 0xFFFFFFFF; // I/O disabled
    }
    
    return m_ide2->readCommandPort(offset);
}

void SiS5513IdeDevice::ide2WriteCs0(int offset, uint32_t data)
{
    if (!(getCommand() & 1)) {
        return; // I/O disabled
    }
    
    m_ide2->writeCommandPort(offset, data);
}

uint8_t SiS5513IdeDevice::ide2ReadCs1()
{
    if (!(getCommand() & 1)) {
        return 0xFF; // I/O disabled
    }
    
    return m_ide2->readControlPort();
}

void SiS5513IdeDevice::ide2WriteCs1(uint8_t data)
{
    if (!(getCommand() & 1)) {
        return; // I/O disabled
    }
    
    m_ide2->writeControlPort(data);
}

} // namespace x86emu