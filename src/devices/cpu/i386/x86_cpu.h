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

#ifndef X86EMULATOR_X86_CPU_H
#define X86EMULATOR_X86_CPU_H

#include "common/i386_interface.h"
#include "x86_cpu_factory.h"
#include <memory>
#include <string>

namespace x86emu {

/**
 * @brief Main CPU class for the x86Emulator
 * 
 * This class provides a unified interface for the CPU, using different backends
 * based on configuration.
 */
class X86CPU {
public:
    /**
     * @brief Construct a new X86CPU
     * 
     * @param cpuModel CPU model (e.g., "i386", "i486", "pentium")
     * @param backendType Backend implementation to use
     */
    X86CPU(const std::string& cpuModel, CPUBackendType backendType = X86CPUFactory::GetDefaultBackendType());
    
    /**
     * @brief Destructor
     */
    ~X86CPU();
    
    /**
     * @brief Initialize the CPU
     * 
     * @return bool True if initialization was successful
     */
    bool Initialize();
    
    /**
     * @brief Reset the CPU
     */
    void Reset();
    
    /**
     * @brief Execute for the specified number of cycles
     * 
     * @param cycles Number of cycles to execute
     * @return int Actual number of cycles executed
     */
    int Execute(int cycles);
    
    /**
     * @brief Stop execution
     */
    void Stop();
    
    /**
     * @brief Pause execution
     */
    void Pause();
    
    /**
     * @brief Resume execution
     */
    void Resume();
    
    /**
     * @brief Set interrupt request line
     * 
     * @param irqLine IRQ line number
     * @param state True to assert, false to deassert
     */
    void SetIRQ(int irqLine, bool state);
    
    /**
     * @brief Set NMI line
     * 
     * @param state True to assert, false to deassert
     */
    void SetNMI(bool state);
    
    /**
     * @brief Set IRQ callback
     * 
     * @param callback Callback function
     */
    void SetIRQCallback(void* callback);
    
    /**
     * @brief Read byte from memory
     * 
     * @param address Memory address
     * @return uint8_t Value read
     */
    uint8_t ReadByte(uint32_t address);
    
    /**
     * @brief Read word from memory
     * 
     * @param address Memory address
     * @return uint16_t Value read
     */
    uint16_t ReadWord(uint32_t address);
    
    /**
     * @brief Read double word from memory
     * 
     * @param address Memory address
     * @return uint32_t Value read
     */
    uint32_t ReadDword(uint32_t address);
    
    /**
     * @brief Write byte to memory
     * 
     * @param address Memory address
     * @param value Value to write
     */
    void WriteByte(uint32_t address, uint8_t value);
    
    /**
     * @brief Write word to memory
     * 
     * @param address Memory address
     * @param value Value to write
     */
    void WriteWord(uint32_t address, uint16_t value);
    
    /**
     * @brief Write double word to memory
     * 
     * @param address Memory address
     * @param value Value to write
     */
    void WriteDword(uint32_t address, uint32_t value);
    
    /**
     * @brief Read from I/O port
     * 
     * @param port I/O port
     * @return uint8_t Value read
     */
    uint8_t ReadIO(uint16_t port);
    
    /**
     * @brief Write to I/O port
     * 
     * @param port I/O port
     * @param value Value to write
     */
    void WriteIO(uint16_t port, uint8_t value);
    
    /**
     * @brief Get register value
     * 
     * @param regIndex Register index
     * @return uint32_t Register value
     */
    uint32_t GetRegister(int regIndex);
    
    /**
     * @brief Set register value
     * 
     * @param regIndex Register index
     * @param value New value
     */
    void SetRegister(int regIndex, uint32_t value);
    
    /**
     * @brief Get disassembly for the current instruction
     * 
     * @return std::string Disassembly string
     */
    std::string GetDisassembly();
    
    /**
     * @brief Get CPU type
     * 
     * @return std::string CPU type string
     */
    std::string GetCPUType() const;
    
    /**
     * @brief Get currently used backend type
     * 
     * @return CPUBackendType Backend type
     */
    CPUBackendType GetBackendType() const;
    
private:
    std::string m_cpuModel;
    CPUBackendType m_backendType;
    std::unique_ptr<I386CPUInterface> m_cpu;
    bool m_initialized;
};

} // namespace x86emu

#endif // X86EMULATOR_X86_CPU_H