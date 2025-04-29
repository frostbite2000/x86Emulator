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

#ifndef X86EMULATOR_INTERRUPT_CONTROLLER_H
#define X86EMULATOR_INTERRUPT_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>

/**
 * @brief Interrupt controller for the emulator
 * 
 * Emulates the 8259A Programmable Interrupt Controller (PIC).
 * Handles IRQ management and interrupt cascading with two PICs.
 */
class InterruptController {
public:
    /**
     * @brief Callback for interrupt handling
     */
    using InterruptCallback = std::function<void(int)>;
    
    /**
     * @brief Construct a new Interrupt Controller
     */
    InterruptController();
    
    /**
     * @brief Destroy the Interrupt Controller
     */
    ~InterruptController();
    
    /**
     * @brief Initialize the interrupt controller
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Reset the interrupt controller
     */
    void reset();
    
    /**
     * @brief Assert an IRQ line
     * 
     * @param irq IRQ number (0-15)
     * @param state true to assert, false to deassert
     */
    void setIRQ(int irq, bool state);
    
    /**
     * @brief Check if an IRQ is pending
     * 
     * @return true if an IRQ is pending
     * @return false if no IRQ is pending
     */
    bool isPending() const;
    
    /**
     * @brief Get the next pending interrupt vector
     * 
     * @return Interrupt vector number, or -1 if none pending
     */
    int getNextInterruptVector();
    
    /**
     * @brief Acknowledge an interrupt
     * 
     * @param irq IRQ number (0-15)
     */
    void acknowledgeInterrupt(int irq);
    
    /**
     * @brief Register callback for interrupt handling
     * 
     * @param callback Callback function
     */
    void registerInterruptCallback(InterruptCallback callback);
    
    /**
     * @brief Read from PIC register
     * 
     * @param port I/O port address
     * @return Register value
     */
    uint8_t readRegister(uint16_t port) const;
    
    /**
     * @brief Write to PIC register
     * 
     * @param port I/O port address
     * @param value Value to write
     */
    void writeRegister(uint16_t port, uint8_t value);
    
    /**
     * @brief Update the interrupt controller
     * 
     * Called once per emulation frame to check and process interrupts
     */
    void update();

private:
    // PIC constants
    static constexpr uint16_t PIC1_COMMAND = 0x20;
    static constexpr uint16_t PIC1_DATA = 0x21;
    static constexpr uint16_t PIC2_COMMAND = 0xA0;
    static constexpr uint16_t PIC2_DATA = 0xA1;
    
    // PIC commands
    static constexpr uint8_t ICW1_ICW4 = 0x01;      // ICW4 needed
    static constexpr uint8_t ICW1_SINGLE = 0x02;    // Single mode
    static constexpr uint8_t ICW1_INTERVAL4 = 0x04; // Call address interval 4
    static constexpr uint8_t ICW1_LEVEL = 0x08;     // Level triggered mode
    static constexpr uint8_t ICW1_INIT = 0x10;      // Initialization command
    
    static constexpr uint8_t ICW4_8086 = 0x01;      // 8086/88 mode
    static constexpr uint8_t ICW4_AUTO = 0x02;      // Auto EOI
    static constexpr uint8_t ICW4_BUF_SLAVE = 0x08; // Buffered mode/slave
    static constexpr uint8_t ICW4_BUF_MASTER = 0x0C; // Buffered mode/master
    static constexpr uint8_t ICW4_SFNM = 0x10;      // Special fully nested mode
    
    static constexpr uint8_t OCW2_EOI = 0x20;       // Non-specific EOI
    static constexpr uint8_t OCW2_SL = 0x40;        // Specific EOI
    static constexpr uint8_t OCW2_ROTATE = 0x80;    // Rotate
    
    static constexpr uint8_t OCW3_READ_IRR = 0x0A;  // Read IRR
    static constexpr uint8_t OCW3_READ_ISR = 0x0B;  // Read ISR
    
    // PIC structure
    struct PIC {
        uint8_t irr;         // Interrupt request register
        uint8_t isr;         // In-service register
        uint8_t imr;         // Interrupt mask register
        uint8_t icw1;        // Initialization command word 1
        uint8_t icw2;        // Initialization command word 2
        uint8_t icw3;        // Initialization command word 3
        uint8_t icw4;        // Initialization command word 4
        bool initState;      // Initialization state
        uint8_t initCount;   // Initialization count
        bool readIsr;        // Read ISR instead of IRR
        uint8_t autoEOI;     // Auto EOI mode
        uint8_t rotateOnAEOI; // Rotate on Auto EOI
        uint8_t priority;    // Priority
    };
    
    PIC m_pic1;  // Master PIC
    PIC m_pic2;  // Slave PIC
    
    // IRQ states
    uint16_t m_irqLines;     // Current IRQ line states
    uint16_t m_irqRequests;  // IRQ requests
    
    // Interrupt callback
    InterruptCallback m_interruptCallback;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;
    
    // Helper methods
    void updatePIC();
    bool picHasPendingInterrupt(const PIC& pic) const;
    int getHighestPriorityIRQ(const PIC& pic) const;
    void acknowledgeInterruptPIC(PIC& pic, int irq);
    void setIRQLine(int irq, bool state);
};

#endif // X86EMULATOR_INTERRUPT_CONTROLLER_H