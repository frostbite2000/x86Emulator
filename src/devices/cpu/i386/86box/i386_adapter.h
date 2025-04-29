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

#ifndef X86EMULATOR_86BOX_I386_ADAPTER_H
#define X86EMULATOR_86BOX_I386_ADAPTER_H

#include "../common/i386_interface.h"
#include "86box_integration.h"
#include <string>

namespace x86emu {

/**
 * @brief Adapter for 86Box's i386 CPU implementation
 * 
 * This class adapts the 86Box i386 CPU implementation to the I386CPUInterface.
 */
class Box86I386Adapter : public I386CPUInterface {
public:
    Box86I386Adapter();
    ~Box86I386Adapter() override;
    
    // CPU identification
    std::string GetCPUType() const override;
    uint32_t GetCPUFlags() const override;
    
    // Lifecycle methods
    bool Initialize() override;
    void Reset() override;
    void Shutdown() override;
    
    // Execution control
    int Execute(int cycles) override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    
    // Interrupt handling
    void AssertIRQ(int irqLine, bool state) override;
    void AssertNMI(bool state) override;
    void SetIRQCallback(void* callback) override;
    
    // Memory access
    uint8_t ReadByte(uint32_t address) override;
    uint16_t ReadWord(uint32_t address) override;
    uint32_t ReadDword(uint32_t address) override;
    void WriteByte(uint32_t address, uint8_t value) override;
    void WriteWord(uint32_t address, uint16_t value) override;
    void WriteDword(uint32_t address, uint32_t value) override;
    
    // I/O port access
    uint8_t ReadIO(uint16_t port) override;
    void WriteIO(uint16_t port, uint8_t value) override;
    
    // Register access
    uint32_t GetRegister(int regIndex) override;
    void SetRegister(int regIndex, uint32_t value) override;
    
    // Special instructions
    void ExecuteHLT() override;
    void ExecuteRDTSC() override;
    void ExecuteCPUID() override;
    
    // Debug support
    std::string GetDisassembly(uint32_t pc) override;
    
private:
    std::string m_cpuModel;
    bool m_initialized;
    bool m_paused;
    char m_disasmBuffer[256];
};

} // namespace x86emu

#endif // X86EMULATOR_86BOX_I386_ADAPTER_H