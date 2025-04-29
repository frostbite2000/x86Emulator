/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
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

#include "interrupt_controller.h"
#include "logger.h"

InterruptController::InterruptController()
    : m_irqLines(0),
      m_irqRequests(0)
{
    // Initialize PICs
    memset(&m_pic1, 0, sizeof(PIC));
    memset(&m_pic2, 0, sizeof(PIC));
}

InterruptController::~InterruptController()
{
}

bool InterruptController::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Reset PICs
        reset();
        
        Logger::GetInstance()->info("Interrupt controller initialized");
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Interrupt controller initialization failed: %s", ex.what());
        return false;
    }
}

void InterruptController::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset master PIC
    m_pic1.irr = 0;
    m_pic1.isr = 0;
    m_pic1.imr = 0xFF; // All interrupts masked
    m_pic1.icw1 = 0;
    m_pic1.icw2 = 0x08; // IRQ 0-7 -> INT 08h-0Fh
    m_pic1.icw3 = 0;
    m_pic1.icw4 = 0;
    m_pic1.initState = false;
    m_pic1.initCount = 0;
    m_pic1.readIsr = false;
    m_pic1.autoEOI = 0;
    m_pic1.rotateOnAEOI = 0;
    m_pic1.priority = 0;
    
    // Reset slave PIC
    m_pic2.irr = 0;
    m_pic2.isr = 0;
    m_pic2.imr = 0xFF; // All interrupts masked
    m_pic2.icw1 = 0;
    m_pic2.icw2 = 0x70; // IRQ 8-15 -> INT 70h-77h
    m_pic2.icw3 = 0;
    m_pic2.icw4 = 0;
    m_pic2.initState = false;
    m_pic2.initCount = 0;
    m_pic2.readIsr = false;
    m_pic2.autoEOI = 0;
    m_pic2.rotateOnAEOI = 0;
    m_pic2.priority = 0;
    
    // Reset IRQ lines
    m_irqLines = 0;
    m_irqRequests = 0;
    
    Logger::GetInstance()->info("Interrupt controller reset");
}

void InterruptController::setIRQ(int irq, bool state)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if IRQ is valid
    if (irq < 0 || irq > 15) {
        Logger::GetInstance()->warn("Invalid IRQ number: %d", irq);
        return;
    }
    
    // Update IRQ line state
    uint16_t mask = 1 << irq;
    bool oldState = (m_irqLines & mask) != 0;
    
    if (state != oldState) {
        // State changed
        if (state) {
            m_irqLines |= mask;
            m_irqRequests |= mask;
            
            Logger::GetInstance()->debug("IRQ %d asserted", irq);
        } else {
            m_irqLines &= ~mask;
            
            Logger::GetInstance()->debug("IRQ %d deasserted", irq);
        }
        
        // Update PICs
        updatePIC();
    }
}

bool InterruptController::isPending() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if master PIC has a pending interrupt
    if (picHasPendingInterrupt(m_pic1)) {
        return true;
    }
    
    // Check if slave PIC has a pending interrupt and is not masked by master
    if (picHasPendingInterrupt(m_pic2) && !(m_pic1.imr & (1 << 2))) {
        return true;
    }
    
    return false;
}

int InterruptController::getNextInterruptVector()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if master PIC has a pending interrupt
    int masterIRQ = getHighestPriorityIRQ(m_pic1);
    if (masterIRQ >= 0) {
        // Check if it's the cascade IRQ (IRQ2)
        if (masterIRQ == 2) {
            // Check if slave PIC has a pending interrupt
            int slaveIRQ = getHighestPriorityIRQ(m_pic2);
            if (slaveIRQ >= 0) {
                // Convert slave IRQ to interrupt vector
                int vector = m_pic2.icw2 + slaveIRQ;
                
                // Set in-service bit
                m_pic2.isr |= (1 << slaveIRQ);
                m_pic1.isr |= (1 << 2);
                
                // Clear request bit
                m_pic2.irr &= ~(1 << slaveIRQ);
                
                Logger::GetInstance()->debug("Interrupt vector 0x%02X (IRQ %d) from slave PIC", vector, slaveIRQ + 8);
                return vector;
            }
        } else {
            // Convert master IRQ to interrupt vector
            int vector = m_pic1.icw2 + masterIRQ;
            
            // Set in-service bit
            m_pic1.isr |= (1 << masterIRQ);
            
            // Clear request bit
            m_pic1.irr &= ~(1 << masterIRQ);
            
            Logger::GetInstance()->debug("Interrupt vector 0x%02X (IRQ %d) from master PIC", vector, masterIRQ);
            return vector;
        }
    }
    
    return -1;
}

void InterruptController::acknowledgeInterrupt(int irq)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if IRQ is valid
    if (irq < 0 || irq > 15) {
        Logger::GetInstance()->warn("Invalid IRQ number: %d", irq);
        return;
    }
    
    // Check which PIC handles this IRQ
    if (irq < 8) {
        // Master PIC
        acknowledgeInterruptPIC(m_pic1, irq);
    } else {
        // Slave PIC
        acknowledgeInterruptPIC(m_pic2, irq - 8);
        // Also acknowledge cascade IRQ on master
        acknowledgeInterruptPIC(m_pic1, 2);
    }
    
    // Clear IRQ request
    m_irqRequests &= ~(1 << irq);
    
    Logger::GetInstance()->debug("IRQ %d acknowledged", irq);
}

void InterruptController::registerInterruptCallback(InterruptCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_interruptCallback = callback;
}

uint8_t InterruptController::readRegister(uint16_t port) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Determine which PIC and register to read
    if (port == PIC1_COMMAND) {
        // Master PIC command port
        return m_pic1.readIsr ? m_pic1.isr : m_pic1.irr;
    }
    else if (port == PIC1_DATA) {
        // Master PIC data port
        return m_pic1.imr;
    }
    else if (port == PIC2_COMMAND) {
        // Slave PIC command port
        return m_pic2.readIsr ? m_pic2.isr : m_pic2.irr;
    }
    else if (port == PIC2_DATA) {
        // Slave PIC data port
        return m_pic2.imr;
    }
    
    Logger::GetInstance()->warn("Invalid PIC register read: 0x%04X", port);
    return 0xFF;
}

void InterruptController::writeRegister(uint16_t port, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Determine which PIC and register to write
    if (port == PIC1_COMMAND) {
        // Master PIC command port
        if (value & ICW1_INIT) {
            // Initialization command
            m_pic1.icw1 = value;
            m_pic1.initState = true;
            m_pic1.initCount = 1;
            m_pic1.imr = 0; // Clear interrupt mask
            m_pic1.isr = 0; // Clear in-service register
            Logger::GetInstance()->debug("Master PIC initialization started");
        }
        else if ((value & 0x18) == 0x08) {
            // OCW3
            if (value & 0x02) {
                m_pic1.readIsr = (value & 0x01);
            }
            Logger::GetInstance()->debug("Master PIC OCW3: 0x%02X", value);
        }
        else if ((value & 0xE0) == 0x20) {
            // OCW2 (EOI command)
            if (value & OCW2_SL) {
                // Specific EOI
                int irq = value & 0x07;
                m_pic1.isr &= ~(1 << irq);
                Logger::GetInstance()->debug("Master PIC specific EOI for IRQ %d", irq);
            } else {
                // Non-specific EOI
                for (int i = 0; i < 8; i++) {
                    if (m_pic1.isr & (1 << i)) {
                        m_pic1.isr &= ~(1 << i);
                        Logger::GetInstance()->debug("Master PIC non-specific EOI for IRQ %d", i);
                        break;
                    }
                }
            }
        }
    }
    else if (port == PIC1_DATA) {
        // Master PIC data port
        if (m_pic1.initState) {
            // Initialization sequence
            switch (m_pic1.initCount) {
                case 1:
                    // ICW2 - Interrupt vector base
                    m_pic1.icw2 = value & 0xF8;
                    m_pic1.initCount++;
                    Logger::GetInstance()->debug("Master PIC ICW2: 0x%02X", value);
                    break;
                case 2:
                    // ICW3 - Cascade configuration
                    m_pic1.icw3 = value;
                    m_pic1.initCount++;
                    Logger::GetInstance()->debug("Master PIC ICW3: 0x%02X", value);
                    break;
                case 3:
                    // ICW4 - Mode
                    m_pic1.icw4 = value;
                    m_pic1.autoEOI = (value & ICW4_AUTO) ? 1 : 0;
                    m_pic1.initState = false;
                    Logger::GetInstance()->debug("Master PIC ICW4: 0x%02X", value);
                    break;
            }
        }
        else {
            // OCW1 - Interrupt mask
            m_pic1.imr = value;
            Logger::GetInstance()->debug("Master PIC mask: 0x%02X", value);
            
            // Update PICs
            updatePIC();
        }
    }
    else if (port == PIC2_COMMAND) {
        // Slave PIC command port
        if (value & ICW1_INIT) {
            // Initialization command
            m_pic2.icw1 = value;
            m_pic2.initState = true;
            m_pic2.initCount = 1;
            m_pic2.imr = 0; // Clear interrupt mask
            m_pic2.isr = 0; // Clear in-service register
            Logger::GetInstance()->debug("Slave PIC initialization started");
        }
        else if ((value & 0x18) == 0x08) {
            // OCW3
            if (value & 0x02) {
                m_pic2.readIsr = (value & 0x01);
            }
            Logger::GetInstance()->debug("Slave PIC OCW3: 0x%02X", value);
        }
        else if ((value & 0xE0) == 0x20) {
            // OCW2 (EOI command)
            if (value & OCW2_SL) {
                // Specific EOI
                int irq = value & 0x07;
                m_pic2.isr &= ~(1 << irq);
                Logger::GetInstance()->debug("Slave PIC specific EOI for IRQ %d", irq + 8);
            } else {
                // Non-specific EOI
                for (int i = 0; i < 8; i++) {
                    if (m_pic2.isr & (1 << i)) {
                        m_pic2.isr &= ~(1 << i);
                        Logger::GetInstance()->debug("Slave PIC non-specific EOI for IRQ %d", i + 8);
                        break;
                    }
                }
            }
        }
    }
    else if (port == PIC2_DATA) {
        // Slave PIC data port
        if (m_pic2.initState) {
            // Initialization sequence
            switch (m_pic2.initCount) {
                case 1:
                    // ICW2 - Interrupt vector base
                    m_pic2.icw2 = value & 0xF8;
                    m_pic2.initCount++;
                    Logger::GetInstance()->debug("Slave PIC ICW2: 0x%02X", value);
                    break;
                case 2:
                    // ICW3 - Cascade configuration
                    m_pic2.icw3 = value;
                    m_pic2.initCount++;
                    Logger::GetInstance()->debug("Slave PIC ICW3: 0x%02X", value);
                    break;
                case 3:
                    // ICW4 - Mode
                    m_pic2.icw4 = value;
                    m_pic2.autoEOI = (value & ICW4_AUTO) ? 1 : 0;
                    m_pic2.initState = false;
                    Logger::GetInstance()->debug("Slave PIC ICW4: 0x%02X", value);
                    break;
            }
        }
        else {
            // OCW1 - Interrupt mask
            m_pic2.imr = value;
            Logger::GetInstance()->debug("Slave PIC mask: 0x%02X", value);
            
            // Update PICs
            updatePIC();
        }
    }
    else {
        Logger::GetInstance()->warn("Invalid PIC register write: 0x%04X = 0x%02X", port, value);
    }
}

void InterruptController::update()
{
    // Check if there are any pending interrupts
    if (isPending() && m_interruptCallback) {
        // Get the next interrupt vector
        int vector = getNextInterruptVector();
        if (vector >= 0) {
            // Call the callback
            m_interruptCallback(vector);
        }
    }
}

void InterruptController::updatePIC()
{
    // Update master PIC IRR based on IRQ requests
    uint8_t masterRequests = m_irqRequests & 0xFF;
    m_pic1.irr = masterRequests & ~m_pic1.imr;
    
    // Update slave PIC IRR based on IRQ requests
    uint8_t slaveRequests = (m_irqRequests >> 8) & 0xFF;
    m_pic2.irr = slaveRequests & ~m_pic2.imr;
    
    // If slave PIC has IRQs pending, assert IRQ2 on master
    if (m_pic2.irr) {
        m_pic1.irr |= (1 << 2) & ~m_pic1.imr;
    }
    
    // Check if we need to notify about a pending interrupt
    if (isPending() && m_interruptCallback) {
        // Just notify that an interrupt is pending, the actual vector
        // will be determined when update() is called
        m_interruptCallback(-1);
    }
}

bool InterruptController::picHasPendingInterrupt(const PIC& pic) const
{
    // Check if any unmasked interrupt is pending
    return (pic.irr & ~pic.imr) != 0;
}

int InterruptController::getHighestPriorityIRQ(const PIC& pic) const
{
    // Get unmasked IRQs
    uint8_t pending = pic.irr & ~pic.imr;
    if (!pending) {
        return -1;
    }
    
    // Find highest priority IRQ (lowest bit set)
    for (int i = 0; i < 8; i++) {
        if (pending & (1 << i)) {
            return i;
        }
    }
    
    return -1;
}

void InterruptController::acknowledgeInterruptPIC(PIC& pic, int irq)
{
    // Clear the in-service bit
    pic.isr &= ~(1 << irq);
    
    // If in auto-EOI mode, also clear the request bit
    if (pic.autoEOI) {
        pic.irr &= ~(1 << irq);
    }
}

void InterruptController::setIRQLine(int irq, bool state)
{
    // Update IRQ line state
    uint16_t mask = 1 << irq;
    bool oldState = (m_irqLines & mask) != 0;
    
    if (state != oldState) {
        // State changed
        if (state) {
            m_irqLines |= mask;
            m_irqRequests |= mask;
        } else {
            m_irqLines &= ~mask;
        }
    }
}