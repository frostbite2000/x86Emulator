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

#include "86box_integration.h"
#include <string.h>
#include <stdio.h>

// Include 86Box CPU headers
extern "C" {
#include "86box/cpu.h"
#include "86box/cpu/x86.h"
#include "86box/machine.h"
#include "86box/io.h"
#include "86box/mem.h"
}

// Global variables
namespace {
    void* g_irqCallback = nullptr;
    bool g_initialized = false;
}

// C++ implementation of wrapper functions
namespace x86emu {
namespace box86 {

bool InitializeCPU(const char* model)
{
    if (g_initialized) {
        return true;
    }
    
    // Initialize 86Box CPU based on model
    int cpu_type = 0;
    
    if (strcmp(model, "i386") == 0 || strcmp(model, "386") == 0) {
        cpu_type = CPU_386DX;
    } else if (strcmp(model, "i486") == 0 || strcmp(model, "486") == 0) {
        cpu_type = CPU_486DX;
    } else if (strstr(model, "pentium") != nullptr || strcmp(model, "p5") == 0) {
        cpu_type = CPU_PENTIUM;
    } else if (strstr(model, "pentium_mmx") != nullptr || strcmp(model, "p55c") == 0) {
        cpu_type = CPU_PENTIUMMMX;
    } else if (strstr(model, "pentium_pro") != nullptr || strcmp(model, "p6") == 0) {
        cpu_type = CPU_PENTIUMPRO;
    } else if (strstr(model, "pentium_ii") != nullptr || strcmp(model, "p2") == 0) {
        cpu_type = CPU_PENTIUM2;
    } else {
        // Default to i386
        cpu_type = CPU_386DX;
    }
    
    // Initialize CPU
    cpu_set(cpu_type);
    
    // Initialize memory (if needed)
    // This would depend on your implementation
    
    g_initialized = true;
    return true;
}

void ResetCPU()
{
    if (g_initialized) {
        cpu_reset();
    }
}

void ShutdownCPU()
{
    if (g_initialized) {
        // Any cleanup needed for 86Box CPU
        g_initialized = false;
    }
}

int ExecuteCPU(int cycles)
{
    if (!g_initialized) {
        return 0;
    }
    
    // 86Box's execute function
    return cpu_exec(cycles);
}

void StopCPU()
{
    if (g_initialized) {
        cpu_set_run(0);
    }
}

void AssertIRQ(int irqLine, bool state)
{
    if (g_initialized) {
        if (state) {
            pic_set_irq(irqLine);
        } else {
            pic_clear_irq(irqLine);
        }
    }
}

void AssertNMI(bool state)
{
    if (g_initialized) {
        nmi = state ? 1 : 0;
    }
}

void SetIRQCallback(void* callback)
{
    g_irqCallback = callback;
}

uint8_t ReadMemoryByte(uint32_t address)
{
    if (!g_initialized) {
        return 0xFF;
    }
    
    return mem_readb_phys(address);
}

uint16_t ReadMemoryWord(uint32_t address)
{
    if (!g_initialized) {
        return 0xFFFF;
    }
    
    return mem_readw_phys(address);
}

uint32_t ReadMemoryDword(uint32_t address)
{
    if (!g_initialized) {
        return 0xFFFFFFFF;
    }
    
    return mem_readl_phys(address);
}

void WriteMemoryByte(uint32_t address, uint8_t value)
{
    if (g_initialized) {
        mem_writeb_phys(address, value);
    }
}

void WriteMemoryWord(uint32_t address, uint16_t value)
{
    if (g_initialized) {
        mem_writew_phys(address, value);
    }
}

void WriteMemoryDword(uint32_t address, uint32_t value)
{
    if (g_initialized) {
        mem_writel_phys(address, value);
    }
}

uint8_t ReadIOByte(uint16_t port)
{
    if (!g_initialized) {
        return 0xFF;
    }
    
    return inb(port);
}

void WriteIOByte(uint16_t port, uint8_t value)
{
    if (g_initialized) {
        outb(port, value);
    }
}

uint32_t GetRegister(int regIndex)
{
    if (!g_initialized) {
        return 0;
    }
    
    // Map register index to 86Box's CPU registers
    switch (regIndex) {
        case 0: return cpu_state.eax;
        case 1: return cpu_state.ebx;
        case 2: return cpu_state.ecx;
        case 3: return cpu_state.edx;
        case 4: return cpu_state.esi;
        case 5: return cpu_state.edi;
        case 6: return cpu_state.ebp;
        case 7: return cpu_state.esp;
        case 8: return cpu_state.pc;
        case 9: return cpu_state.flags;
        // Add more registers as needed
        default: return 0;
    }
}

void SetRegister(int regIndex, uint32_t value)
{
    if (!g_initialized) {
        return;
    }
    
    // Map register index to 86Box's CPU registers
    switch (regIndex) {
        case 0: cpu_state.eax = value; break;
        case 1: cpu_state.ebx = value; break;
        case 2: cpu_state.ecx = value; break;
        case 3: cpu_state.edx = value; break;
        case 4: cpu_state.esi = value; break;
        case 5: cpu_state.edi = value; break;
        case 6: cpu_state.ebp = value; break;
        case 7: cpu_state.esp = value; break;
        case 8: cpu_state.pc = value; break;
        case 9: cpu_state.flags = value; break;
        // Add more registers as needed
    }
}

const char* GetDisassembly(uint32_t pc, char* buffer, size_t buffer_size)
{
    if (!g_initialized || buffer == nullptr) {
        return "";
    }
    
    // This would depend on 86Box's disassembly functions
    // For now, just return a placeholder
    snprintf(buffer, buffer_size, "Instruction at 0x%08X", pc);
    return buffer;
}

} // namespace box86
} // namespace x86emu

// C function implementations
extern "C" {

uint8_t x86emu_box86_read_memory_byte(uint32_t address)
{
    // Forward to your memory system
    return x86emu::box86::ReadMemoryByte(address);
}

uint16_t x86emu_box86_read_memory_word(uint32_t address)
{
    return x86emu::box86::ReadMemoryWord(address);
}

uint32_t x86emu_box86_read_memory_dword(uint32_t address)
{
    return x86emu::box86::ReadMemoryDword(address);
}

void x86emu_box86_write_memory_byte(uint32_t address, uint8_t value)
{
    x86emu::box86::WriteMemoryByte(address, value);
}

void x86emu_box86_write_memory_word(uint32_t address, uint16_t value)
{
    x86emu::box86::WriteMemoryWord(address, value);
}

void x86emu_box86_write_memory_dword(uint32_t address, uint32_t value)
{
    x86emu::box86::WriteMemoryDword(address, value);
}

uint8_t x86emu_box86_read_io_byte(uint16_t port)
{
    // Forward to your I/O system
    return x86emu::box86::ReadIOByte(port);
}

void x86emu_box86_write_io_byte(uint16_t port, uint8_t value)
{
    x86emu::box86::WriteIOByte(port, value);
}

int x86emu_box86_irq_callback(int irqLine)
{
    // Call the registered IRQ callback if available
    if (g_irqCallback) {
        // Cast to appropriate function type and call
        // This would depend on your callback signature
        // For now, just return the irqLine
        return irqLine;
    }
    return -1;
}

} // extern "C"