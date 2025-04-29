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

#include "i386_adapter.h"
#include "86box_integration.h"

namespace x86emu {

Box86I386Adapter::Box86I386Adapter()
    : m_cpuModel("i386")
    , m_initialized(false)
    , m_paused(false)
{
}

Box86I386Adapter::~Box86I386Adapter()
{
    Shutdown();
}

std::string Box86I386Adapter::GetCPUType() const
{
    return m_cpuModel;
}

uint32_t Box86I386Adapter::GetCPUFlags() const
{
    // This would be mapped to 86Box CPU flags
    return 0;
}

bool Box86I386Adapter::Initialize()
{
    if (m_initialized) {
        return true;
    }
    
    bool result = box86::InitializeCPU(m_cpuModel.c_str());
    if (result) {
        m_initialized = true;
    }
    
    return result;
}

void Box86I386Adapter::Reset()
{
    if (m_initialized) {
        box86::ResetCPU();
    }
}

void Box86I386Adapter::Shutdown()
{
    if (m_initialized) {
        box86::ShutdownCPU();
        m_initialized = false;
    }
}

int Box86I386Adapter::Execute(int cycles)
{
    if (!m_initialized || m_paused) {
        return 0;
    }
    
    return box86::ExecuteCPU(cycles);
}

void Box86I386Adapter::Stop()
{
    if (m_initialized) {
        box86::StopCPU();
    }
}

void Box86I386Adapter::Pause()
{
    m_paused = true;
}

void Box86I386Adapter::Resume()
{
    m_paused = false;
}

void Box86I386Adapter::AssertIRQ(int irqLine, bool state)
{
    if (m_initialized) {
        box86::AssertIRQ(irqLine, state);
    }
}

void Box86I386Adapter::AssertNMI(bool state)
{
    if (m_initialized) {
        box86::AssertNMI(state);
    }
}

void Box86I386Adapter::SetIRQCallback(void* callback)
{
    if (m_initialized) {
        box86::SetIRQCallback(callback);
    }
}

uint8_t Box86I386Adapter::ReadByte(uint32_t address)
{
    if (!m_initialized) {
        return 0xFF;
    }
    
    return box86::ReadMemoryByte(address);
}

uint16_t Box86I386Adapter::ReadWord(uint32_t address)
{
    if (!m_initialized) {
        return 0xFFFF;
    }
    
    return box86::ReadMemoryWord(address);
}

uint32_t Box86I386Adapter::ReadDword(uint32_t address)
{
    if (!m_initialized) {
        return 0xFFFFFFFF;
    }
    
    return box86::ReadMemoryDword(address);
}

void Box86I386Adapter::WriteByte(uint32_t address, uint8_t value)
{
    if (m_initialized) {
        box86::WriteMemoryByte(address, value);
    }
}

void Box86I386Adapter::WriteWord(uint32_t address, uint16_t value)
{
    if (m_initialized) {
        box86::WriteMemoryWord(address, value);
    }
}

void Box86I386Adapter::WriteDword(uint32_t address, uint32_t value)
{
    if (m_initialized) {
        box86::WriteMemoryDword(address, value);
    }
}

uint8_t Box86I386Adapter::ReadIO(uint16_t port)
{
    if (!m_initialized) {
        return 0xFF;
    }
    
    return box86::ReadIOByte(port);
}

void Box86I386Adapter::WriteIO(uint16_t port, uint8_t value)
{
    if (m_initialized) {
        box86::WriteIOByte(port, value);
    }
}

uint32_t Box86I386Adapter::GetRegister(int regIndex)
{
    if (!m_initialized) {
        return 0;
    }
    
    return box86::GetRegister(regIndex);
}

void Box86I386Adapter::SetRegister(int regIndex, uint32_t value)
{
    if (m_initialized) {
        box86::SetRegister(regIndex, value);
    }
}

void Box86I386Adapter::ExecuteHLT()
{
    // Not directly exposed by our 86Box integration layer
    // Could be implemented by setting a special state or flag
}

void Box86I386Adapter::ExecuteRDTSC()
{
    // Not directly exposed by our 86Box integration layer
    // Could be implemented by calling a specific function
}

void Box86I386Adapter::ExecuteCPUID()
{
    // Not directly exposed by our 86Box integration layer
    // Could be implemented by calling a specific function
}

std::string Box86I386Adapter::GetDisassembly(uint32_t pc)
{
    if (!m_initialized) {
        return "";
    }
    
    box86::GetDisassembly(pc, m_disasmBuffer, sizeof(m_disasmBuffer));
    return std::string(m_disasmBuffer);
}

} // namespace x86emu