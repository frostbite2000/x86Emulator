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

#include "x86_cpu.h"
#include "x86_cpu_factory.h"
#include <stdexcept>

namespace x86emu {

X86CPU::X86CPU(const std::string& cpuModel, CPUBackendType backendType)
    : m_cpuModel(cpuModel)
    , m_backendType(backendType)
    , m_initialized(false)
{
    // Check if the requested backend is available
    if (!X86CPUFactory::IsBackendAvailable(backendType)) {
        // Fall back to the default backend
        m_backendType = X86CPUFactory::GetDefaultBackendType();
    }
    
    // Create the CPU instance
    m_cpu = X86CPUFactory::CreateCPU(m_backendType, m_cpuModel);
    
    if (!m_cpu) {
        throw std::runtime_error("Failed to create CPU instance");
    }
}

X86CPU::~X86CPU()
{
    if (m_initialized) {
        m_cpu->Shutdown();
    }
}

bool X86CPU::Initialize()
{
    if (m_initialized) {
        return true;
    }
    
    if (m_cpu->Initialize()) {
        m_initialized = true;
        return true;
    }
    
    return false;
}

void X86CPU::Reset()
{
    if (m_initialized) {
        m_cpu->Reset();
    }
}

int X86CPU::Execute(int cycles)
{
    if (!m_initialized) {
        return 0;
    }
    
    return m_cpu->Execute(cycles);
}

void X86CPU::Stop()
{
    if (m_initialized) {
        m_cpu->Stop();
    }
}

void X86CPU::Pause()
{
    if (m_initialized) {
        m_cpu->Pause();
    }
}

void X86CPU::Resume()
{
    if (m_initialized) {
        m_cpu->Resume();
    }
}

void X86CPU::SetIRQ(int irqLine, bool state)
{
    if (m_initialized) {
        m_cpu->AssertIRQ(irqLine, state);
    }
}

void X86CPU::SetNMI(bool state)
{
    if (m_initialized) {
        m_cpu->AssertNMI(state);
    }
}

void X86CPU::SetIRQCallback(void* callback)
{
    if (m_initialized) {
        m_cpu->SetIRQCallback(callback);
    }
}

uint8_t X86CPU::ReadByte(uint32_t address)
{
    if (!m_initialized) {
        return 0xFF;
    }
    
    return m_cpu->ReadByte(address);
}

uint16_t X86CPU::ReadWord(uint32_t address)
{
    if (!m_initialized) {
        return 0xFFFF;
    }
    
    return m_cpu->ReadWord(address);
}

uint32_t X86CPU::ReadDword(uint32_t address)
{
    if (!m_initialized) {
        return 0xFFFFFFFF;
    }
    
    return m_cpu->ReadDword(address);
}

void X86CPU::WriteByte(uint32_t address, uint8_t value)
{
    if (m_initialized) {
        m_cpu->WriteByte(address, value);
    }
}

void X86CPU::WriteWord(uint32_t address, uint16_t value)
{
    if (m_initialized) {
        m_cpu->WriteWord(address, value);
    }
}

void X86CPU::WriteDword(uint32_t address, uint32_t value)
{
    if (m_initialized) {
        m_cpu->WriteDword(address, value);
    }
}

uint8_t X86CPU::ReadIO(uint16_t port)
{
    if (!m_initialized) {
        return 0xFF;
    }
    
    return m_cpu->ReadIO(port);
}

void X86CPU::WriteIO(uint16_t port, uint8_t value)
{
    if (m_initialized) {
        m_cpu->WriteIO(port, value);
    }
}

uint32_t X86CPU::GetRegister(int regIndex)
{
    if (!m_initialized) {
        return 0;
    }
    
    return m_cpu->GetRegister(regIndex);
}

void X86CPU::SetRegister(int regIndex, uint32_t value)
{
    if (m_initialized) {
        m_cpu->SetRegister(regIndex, value);
    }
}

std::string X86CPU::GetDisassembly()
{
    if (!m_initialized) {
        return "";
    }
    
    uint32_t pc = m_cpu->GetRegister(0); // Assuming register 0 is the PC/EIP
    return m_cpu->GetDisassembly(pc);
}

std::string X86CPU::GetCPUType() const
{
    if (!m_initialized) {
        return m_cpuModel;
    }
    
    return m_cpu->GetCPUType();
}

CPUBackendType X86CPU::GetBackendType() const
{
    return m_backendType;
}

} // namespace x86emu