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

#ifndef X86EMULATOR_86BOX_INTEGRATION_H
#define X86EMULATOR_86BOX_INTEGRATION_H

#include <stdint.h>
#include <stddef.h>

// Define C++ wrapper functions for 86Box's CPU implementation
#ifdef __cplusplus
namespace x86emu {
namespace box86 {

// C++ interface functions
bool InitializeCPU(const char* model);
void ResetCPU();
void ShutdownCPU();
int ExecuteCPU(int cycles);
void StopCPU();
void AssertIRQ(int irqLine, bool state);
void AssertNMI(bool state);
void SetIRQCallback(void* callback);
uint8_t ReadMemoryByte(uint32_t address);
uint16_t ReadMemoryWord(uint32_t address);
uint32_t ReadMemoryDword(uint32_t address);
void WriteMemoryByte(uint32_t address, uint8_t value);
void WriteMemoryWord(uint32_t address, uint16_t value);
void WriteMemoryDword(uint32_t address, uint32_t value);
uint8_t ReadIOByte(uint16_t port);
void WriteIOByte(uint16_t port, uint8_t value);
uint32_t GetRegister(int regIndex);
void SetRegister(int regIndex, uint32_t value);
const char* GetDisassembly(uint32_t pc, char* buffer, size_t buffer_size);

} // namespace box86
} // namespace x86emu
#endif // __cplusplus

// C functions that will be implemented in 86box_integration.cpp
// These functions will be called by the 86Box CPU code
#ifdef __cplusplus
extern "C" {
#endif

// Memory access
uint8_t x86emu_box86_read_memory_byte(uint32_t address);
uint16_t x86emu_box86_read_memory_word(uint32_t address);
uint32_t x86emu_box86_read_memory_dword(uint32_t address);
void x86emu_box86_write_memory_byte(uint32_t address, uint8_t value);
void x86emu_box86_write_memory_word(uint32_t address, uint16_t value);
void x86emu_box86_write_memory_dword(uint32_t address, uint32_t value);

// I/O access
uint8_t x86emu_box86_read_io_byte(uint16_t port);
void x86emu_box86_write_io_byte(uint16_t port, uint8_t value);

// IRQ handling
int x86emu_box86_irq_callback(int irqLine);

#ifdef __cplusplus
}
#endif

#endif // X86EMULATOR_86BOX_INTEGRATION_H