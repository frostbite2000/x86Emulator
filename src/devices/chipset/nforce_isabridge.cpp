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
#include "devices/cpu/cpu.h"
#include "log.h"

NFORCE_ISABridge::NFORCE_ISABridge(const DeviceConfig& config)
    : PCIDevice(config),
      IRQController(),
      m_pm1Status(0),
      m_pm1Enable(0),
      m_pm1Control(0),
      m_pm1Timer(0),
      m_gpe0Status(0),
      m_gpe0Enable(0),
      m_globalSmiControl(0),
      m_smiCommandPort(0),
      m_speaker(0),
      m_refresh(false),
      m_pitOut2(1),
      m_spkrData(0),
      m_channelCheck(0),
      m_pic1(nullptr),
      m_pic2(nullptr),
      m_dma1(nullptr),
      m_dma2(nullptr),
      m_pit(nullptr),
      m_rtc(nullptr),
      m_cpu(nullptr),
      m_interruptOutputState(false),
      m_dmaEop(false)
{
    // Initialize GPIO mode registers
    memset(m_gpioMode, 0, sizeof(m_gpioMode));
    
    // Initialize DMA page registers
    memset(m_dmaPage, 0, sizeof(m_dmaPage));
    m_dmaHighByte = 0xFF;
    m_dmaChannel = -1;
    m_pageOffset = 0;
}

bool NFORCE_ISABridge::Initialize() {
    if (IsInitialized()) {
        return true;
    }
    
    // Initialize as a PCI device
    if (!PCIDevice::Initialize()) {
        return false;
    }
    
    // Set up I/O space - This will be a fixed BAR at 0x8000 for ACPI/LPC operations
    m_pciConfig[PCI_REG_BAR0 / 4] = 0x8001;  // I/O space
    
    // Set initial status
    m_pciConfig[PCI_REG_STATUS / 4] = 0x00B0;
    
    // Set initial command
    m_pciConfig[PCI_REG_COMMAND / 4] = 0x0007;  // I/O space, memory space, bus master access enabled
    
    // Set interrupt pin
    m_pciConfig[PCI_REG_INTERRUPT_PIN / 4] = 0x01;  // INTA#
    
    // Create the PIC (8259) controllers
    m_pic1 = new PIC8259();
    m_pic2 = new PIC8259();
    
    // Set up cascading between the two PICs
    m_pic1->SetCascadeInput(2, m_pic2);
    
    // Create the DMA (8237) controllers
    m_dma1 = new I8237DMA();
    m_dma2 = new I8237DMA();
    
    // Set up cascading between the two DMA controllers
    m_dma2->SetDreq0(m_dma1);
    
    // Create the PIT (8254) timer
    m_pit = new PIT8254();
    
    // Connect PIT channel 0 to PIC IRQ 0
    // m_pit->SetIrqLine(0, m_pic1, 0);
    
    // Create the RTC (DS12885)
    m_rtc = new DS12885();
    
    // Connect RTC to PIC IRQ 8
    // m_rtc->SetIrqLine(m_pic2, 0);
    
    // Set multifunction bit in header type
    m_pciConfig[PCI_REG_HEADER_TYPE / 4] |= 0x80;
    
    SetInitialized(true);
    return true;
}

void NFORCE_ISABridge::Reset() {
    PCIDevice::Reset();
    
    // Reset ACPI registers
    m_pm1Status = 0;
    m_pm1Enable = 0;
    m_pm1Control = 0;
    m_pm1Timer = 0;
    m_gpe0Status = 0;
    m_gpe0Enable = 0;
    m_globalSmiControl = 0;
    m_smiCommandPort = 0;
    
    // Reset GPIO mode registers
    memset(m_gpioMode, 0, sizeof(m_gpioMode));
    
    // Reset speaker control
    m_speaker = 0;
    m_refresh = false;
    m_pitOut2 = 1;
    m_spkrData = 0;
    m_channelCheck = 0;
    
    // Reset DMA state
    m_dmaChannel = -1;
    m_dmaHighByte = 0xFF;
    memset(m_dmaPage, 0, sizeof(m_dmaPage));
    m_pageOffset = 0;
    m_dmaEop = false;
    
    // Reset all devices
    if (m_pic1) m_pic1->Reset();
    if (m_pic2) m_pic2->Reset();
    if (m_dma1) m_dma1->Reset();
    if (m_dma2) m_dma2->Reset();
    if (m_pit) m_pit->Reset();
    if (m_rtc) m_rtc->Reset();
    
    m_interruptOutputState = false;
    
    // Reset IRQ controller state
    IRQController::Reset();
}

uint32_t NFORCE_ISABridge::ReadConfig(uint8_t reg, int size) {
    return PCIDevice::ReadConfig(reg, size);
}

void NFORCE_ISABridge::WriteConfig(uint8_t reg, uint32_t value, int size) {
    PCIDevice::WriteConfig(reg, value, size);
}

uint32_t NFORCE_ISABridge::ReadMemory(uint32_t address, int size) {
    return PCIDevice::ReadMemory(address, size);
}

void NFORCE_ISABridge::WriteMemory(uint32_t address, uint32_t value, int size) {
    PCIDevice::WriteMemory(address, value, size);
}

uint32_t NFORCE_ISABridge::ReadIO(uint16_t port, int size) {
    uint32_t value = 0;
    
    // ACPI I/O range - base typically at 0x8000
    uint16_t acpi_base = m_pciConfig[PCI_REG_BAR0 / 4] & 0xFFF0;
    uint16_t acpi_offset = port - acpi_base;
    
    if (port >= acpi_base && port <= acpi_base + 0xFF) {
        return ReadACPI(acpi_offset, size_to_mask(size));
    }
    
    // Check for standard I/O port access
    switch (port) {
        // 8237 DMA controller 1 (channels 0-3)
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
            if (m_dma1) return m_dma1->ReadPort(port);
            break;
            
        // 8259 PIC 1
        case 0x20: case 0x21:
            if (m_pic1) return m_pic1->ReadPort(port - 0x20);
            break;
            
        // 8254 PIT
        case 0x40: case 0x41: case 0x42: case 0x43:
            if (m_pit) return m_pit->ReadPort(port - 0x40);
            break;
            
        // Port B - PIT/Refresh/Speaker/NMI control
        case 0x61:
            value = m_speaker;
            value &= ~0xD0; // AT BIOS doesn't like these bits set
            value |= m_refresh ? 0x10 : 0;  // Refresh bit
            value |= m_pitOut2 ? 0x20 : 0;  // PIT channel 2 output
            return value;
            
        // 8259 PIC 2
        case 0xA0: case 0xA1:
            if (m_pic2) return m_pic2->ReadPort(port - 0xA0);
            break;
            
        // 8237 DMA controller 2 (channels 4-7)
        case 0xC0: case 0xC1: case 0xC2: case 0xC3:
        case 0xC4: case 0xC5: case 0xC6: case 0xC7:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        case 0xD0: case 0xD1: case 0xD2: case 0xD3:
        case 0xD4: case 0xD5: case 0xD6: case 0xD7:
        case 0xD8: case 0xD9: case 0xDA: case 0xDB:
        case 0xDC: case 0xDD: case 0xDE: case 0xDF:
            if (m_dma2) return m_dma2->ReadPort(port - 0xC0);
            break;
            
        // DMA page registers
        case 0x81: case 0x82: case 0x83: case 0x87:  // DMA 0-3
        case 0x89: case 0x8A: case 0x8B: case 0x8F:  // DMA 4-7
            return m_dmaPage[port - 0x80];
            
        // RTC registers
        case 0x70: case 0x71: case 0x72: case 0x73:
            if (m_rtc) return m_rtc->ReadPort(port - 0x70);
            break;
            
        // POST diagnostic port (used for status)
        case 0x80:
            return 0; // Always return success
            
        default:
            // Unknown port
            LOG_DEBUG("nForce ISA Bridge: Read from unknown port %04X\n", port);
            break;
    }
    
    return 0xFF;
}

void NFORCE_ISABridge::WriteIO(uint16_t port, uint32_t value, int size) {
    // ACPI I/O range - base typically at 0x8000
    uint16_t acpi_base = m_pciConfig[PCI_REG_BAR0 / 4] & 0xFFF0;
    uint16_t acpi_offset = port - acpi_base;
    
    if (port >= acpi_base && port <= acpi_base + 0xFF) {
        WriteACPI(acpi_offset, value, size_to_mask(size));
        return;
    }
    
    // Check for standard I/O port access
    switch (port) {
        // 8237 DMA controller 1 (channels 0-3)
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            if (m_dma1) m_dma1->WritePort(port, value & 0xFF);
            break;
            
        // 8259 PIC 1
        case 0x20: case 0x21:
            if (m_pic1) m_pic1->WritePort(port - 0x20, value & 0xFF);
            break;
            
        // 8254 PIT
        case 0x40: case 0x41: case 0x42: case 0x43:
            if (m_pit) m_pit->WritePort(port - 0x40, value & 0xFF);
            break;
            
        // Port B - PIT/Refresh/Speaker/NMI control
        case 0x61:
            m_speaker = value & 0xFF;
            if (m_pit) m_pit->SetGate(2, (value & 0x01) != 0);
            m_spkrData = (value & 0x02) != 0;
            m_channelCheck = (value & 0x08) != 0;
            // TODO: Implement speaker output
            break;
            
        // 8259 PIC 2
        case 0xA0: case 0xA1:
            if (m_pic2) m_pic2->WritePort(port - 0xA0, value & 0xFF);
            break;
            
        // 8237 DMA controller 2 (channels 4-7)
        case 0xC0: case 0xC1: case 0xC2: case 0xC3:
        case 0xC4: case 0xC5: case 0xC6: case 0xC7:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        case 0xD0: case 0xD1: case 0xD2: case 0xD3:
        case 0xD4: case 0xD5: case 0xD6: case 0xD7:
        case 0xD8: case 0xD9: case 0xDA: case 0xDB:
        case 0xDC: case 0xDD: case 0xDE: case 0xDF:
            if (m_dma2) m_dma2->WritePort(port - 0xC0, value & 0xFF);
            break;
            
        // DMA page registers
        case 0x81: case 0x82: case 0x83: case 0x87:  // DMA 0-3
        case 0x89: case 0x8A: case 0x8B: case 0x8F:  // DMA 4-7
            m_dmaPage[port - 0x80] = value & 0xFF;
            break;
            
        // RTC registers
        case 0x70: case 0x71: case 0x72: case 0x73:
            if (m_rtc) m_rtc->WritePort(port - 0x70, value & 0xFF);
            break;
            
        // POST diagnostic port
        case 0x80:
            BootStateCallback(value & 0xFF);
            break;
            
        default:
            // Unknown port
            LOG_DEBUG("nForce ISA Bridge: Write to unknown port %04X = %02X\n", port, value & 0xFF);
            break;
    }
}

uint32_t NFORCE_ISABridge::AcknowledgeIRQ() {
    if (m_pic1) {
        return m_pic1->Acknowledge();
    }
    return 0xFF; // No interrupt
}

void NFORCE_ISABridge::AssertIRQ(int irqNum) {
    if (irqNum < 8) {
        if (m_pic1) {
            m_pic1->AssertIRQ(irqNum);
            UpdateInterrupts();
        }
    } else if (irqNum < 16) {
        if (m_pic2) {
            m_pic2->AssertIRQ(irqNum - 8);
            UpdateInterrupts();
        }
    }
}

void NFORCE_ISABridge::DeassertIRQ(int irqNum) {
    if (irqNum < 8) {
        if (m_pic1) {
            m_pic1->DeassertIRQ(irqNum);
            UpdateInterrupts();
        }
    } else if (irqNum < 16) {
        if (m_pic2) {
            m_pic2->DeassertIRQ(irqNum - 8);
            UpdateInterrupts();
        }
    }
}

void NFORCE_ISABridge::UpdateInterrupts() {
    bool newState = m_pic1 ? m_pic1->HasPendingInterrupt() : false;
    
    if (newState != m_interruptOutputState) {
        m_interruptOutputState = newState;
        
        if (m_cpu) {
            if (newState) {
                m_cpu->AssertIRQ();
            } else {
                m_cpu->DeassertIRQ();
            }
        }
    }
}

uint32_t NFORCE_ISABridge::ReadACPI(uint32_t offset, uint32_t mask) {
    uint32_t ret = 0;
    
    LOG_DEBUG("nForce ISA Bridge: ACPI read from %04X mask %08X\n", offset, mask);
    
    if (offset == 0x00 && (mask & 0xFFFF)) {
        // PM1 status register
        ret |= m_pm1Status & 0xFFFF;
    }
    if (offset == 0x00 && (mask & 0xFFFF0000)) {
        // PM1 enable register
        ret |= (m_pm1Enable << 16);
    }
    if (offset == 0x04 && (mask & 0xFFFF)) {
        // PM1 control register
        ret |= m_pm1Control & 0xFFFF;
    }
    if (offset == 0x08 && (mask & 0xFFFF)) {
        // PM1 timer register
        ret |= m_pm1Timer & 0xFFFF;
    }
    if (offset == 0x20 && (mask & 0xFFFF)) {
        // GPE0 status register
        ret |= m_gpe0Status & 0xFFFF;
    }
    if (offset == 0x20 && (mask & 0xFFFF0000)) {
        // GPE0 enable register
        ret |= (m_gpe0Enable << 16);
    }
    if (offset == 0x28 && (mask & 0xFFFF)) {
        // Global SMI control register
        ret |= m_globalSmiControl & 0xFFFF;
        
        // Bit REGRST (0x02000000) is cleared automatically
        ret &= ~0x02000000;
    }
    if (offset == 0x2C && (mask & 0xFF0000)) {
        // SMI command port
        ret |= (m_smiCommandPort << 16);
    }
    if (offset == 0x30 && (mask & 0x0000FFFF)) {
        // PCRTC_INTR_EN - interrupt enable
        // Register handled by the main PCRTC implementation
    }
    
    return ret;
}

void NFORCE_ISABridge::WriteACPI(uint32_t offset, uint32_t data, uint32_t mask) {
    LOG_DEBUG("nForce ISA Bridge: ACPI write to %04X = %08X mask %08X\n", offset, data, mask);
    
    if (offset == 0x00 && (mask & 0xFFFF)) {
        // PM1 status register
        // Writing 1 to a bit clears it
        m_pm1Status &= ~(data & 0xFFFF);
    }
    if (offset == 0x00 && (mask & 0xFFFF0000)) {
        // PM1 enable register
        m_pm1Enable = (data >> 16) & 0xFFFF;
    }
    if (offset == 0x04 && (mask & 0xFFFF)) {
        // PM1 control register
        m_pm1Control = data & 0xFFFF;
    }
    if (offset == 0x08 && (mask & 0xFFFF)) {
        // PM1 timer register
        m_pm1Timer = data & 0xFFFF;
    }
    if (offset == 0x20 && (mask & 0xFFFF)) {
        // GPE0 status register
        // Writing 1 to a bit clears it
        m_gpe0Status &= ~(data & 0xFFFF);
    }
    if (offset == 0x20 && (mask & 0xFFFF0000)) {
        // GPE0 enable register
        m_gpe0Enable = (data >> 16) & 0xFFFF;
    }
    if (offset == 0x28 && (mask & 0xFFFF)) {
        // Global SMI control register
        // Writing 1 to a bit clears it
        m_globalSmiControl &= ~(data & 0xFFFF);
        
        // Check if we need to generate SMI
        if (m_cpu && m_globalSmiControl) {
            m_cpu->AssertSMI();
        } else if (m_cpu) {
            m_cpu->DeassertSMI();
        }
    }
    if (offset == 0x2C && (mask & 0xFF0000)) {
        // SMI command port
        m_smiCommandPort = (data >> 16) & 0xFF;
        
        // Writing to this port generally triggers an SMI
        m_globalSmiControl |= 0x200;  // Set SMI pending flag
        
        // Generate SMI interrupt
        if (m_cpu) {
            m_cpu->AssertSMI();
        }
        
        LOG_DEBUG("nForce ISA Bridge: Generated software SMI with value %02X\n", m_smiCommandPort);
    }
    if (offset >= 0xC0 && offset < 0xD8 && (mask != 0)) {
        // GPIO mode/configuration registers
        int addr = (offset - 0xC0) / 4;
        if (addr < 6) {  // We have 6 registers, each controlling up to 4 GPIOs
            uint32_t oldValue = m_gpioMode[addr];
            uint32_t newValue = (oldValue & ~mask) | (data & mask);
            m_gpioMode[addr] = newValue;
            
            LOG_DEBUG("nForce ISA Bridge: GPIO mode register %d changed from %08X to %08X\n", 
                    addr, oldValue, newValue);
        }
    }
}

void NFORCE_ISABridge::BootStateCallback(uint8_t data) {
    // This function is called when a value is written to port 0x80
    // These are often BIOS POST codes
    
    // Based on Award BIOS codes
    switch (data) {
        case 0xC0:
            LOG_INFO("BIOS POST: First basic initialization\n");
            break;
        case 0x00:
            LOG_INFO("BIOS POST: Start of POST\n");
            break;
        default:
            LOG_DEBUG("BIOS POST: Code %02X\n", data);
            break;
    }
}

void NFORCE_ISABridge::PITChannel0Changed(bool state) {
    // PIT channel 0 is connected to IRQ 0
    if (state) {
        AssertIRQ(0);
    } else {
        DeassertIRQ(0);
    }
}

void NFORCE_ISABridge::PITChannel1Changed(bool state) {
    // Channel 1 handles DRAM refresh
    if (state) {
        m_refresh = !m_refresh;
    }
}

void NFORCE_ISABridge::PITChannel2Changed(bool state) {
    // Channel 2 is connected to the PC speaker
    m_pitOut2 = state ? 1 : 0;
    // TODO: Implement speaker output
    // Need to combine m_spkrData and m_pitOut2 for actual speaker output
}