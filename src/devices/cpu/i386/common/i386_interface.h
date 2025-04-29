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

#ifndef X86EMULATOR_I386_INTERFACE_H
#define X86EMULATOR_I386_INTERFACE_H

#include <cstdint>
#include <string>

namespace x86emu {

/**
 * @brief Interface for x86 CPU implementations
 * 
 * This interface defines the common methods that all x86 CPU implementations
 * must provide, allowing seamless switching between different backends.
 */
class I386CPUInterface {
public:
    virtual ~I386CPUInterface() = default;
    
    // CPU identification
    virtual std::string GetCPUType() const = 0;
    virtual uint32_t GetCPUFlags() const = 0;
    
    // Lifecycle methods
    virtual bool Initialize() = 0;
    virtual void Reset() = 0;
    virtual void Shutdown() = 0;
    
    // Execution control
    virtual int Execute(int cycles) = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    
    // Interrupt handling
    virtual void AssertIRQ(int irqLine, bool state) = 0;
    virtual void AssertNMI(bool state) = 0;
    virtual void SetIRQCallback(void* callback) = 0;
    
    // Memory access
    virtual uint8_t ReadByte(uint32_t address) = 0;
    virtual uint16_t ReadWord(uint32_t address) = 0;
    virtual uint32_t ReadDword(uint32_t address) = 0;
    virtual void WriteByte(uint32_t address, uint8_t value) = 0;
    virtual void WriteWord(uint32_t address, uint16_t value) = 0;
    virtual void WriteDword(uint32_t address, uint32_t value) = 0;
    
    // I/O port access
    virtual uint8_t ReadIO(uint16_t port) = 0;
    virtual void WriteIO(uint16_t port, uint8_t value) = 0;
    
    // Register access
    virtual uint32_t GetRegister(int regIndex) = 0;
    virtual void SetRegister(int regIndex, uint32_t value) = 0;
    
    // Special instructions
    virtual void ExecuteHLT() = 0;
    virtual void ExecuteRDTSC() = 0;
    virtual void ExecuteCPUID() = 0;
    
    // Debug support
    virtual std::string GetDisassembly(uint32_t pc) = 0;
};

} // namespace x86emu

#endif // X86EMULATOR_I386_INTERFACE_H