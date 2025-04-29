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

#include "x86_cpu_factory.h"
#include "mame/i386_adapter.h"
#include "86box/i386_adapter.h"

namespace x86emu {

std::unique_ptr<I386CPUInterface> X86CPUFactory::CreateCPU(CPUBackendType type, const std::string& cpuModel)
{
    // Create the appropriate CPU instance based on the backend type
    switch (type)
    {
    case CPUBackendType::MAME:
        return std::make_unique<MAMEI386Adapter>();
        
    case CPUBackendType::BOX86:
        return std::make_unique<Box86I386Adapter>();
        
    case CPUBackendType::CUSTOM:
        // Your custom CPU implementation would go here
        break;
    }
    
    // Default to MAME if the requested backend is not available
    return std::make_unique<MAMEI386Adapter>();
}

CPUBackendType X86CPUFactory::GetDefaultBackendType()
{
    // Determine the default backend based on compile-time flags or other criteria
#ifdef USE_MAME_BACKEND
    return CPUBackendType::MAME;
#elif defined(USE_86BOX_BACKEND)
    return CPUBackendType::BOX86;
#else
    return CPUBackendType::MAME; // Default to MAME
#endif
}

bool X86CPUFactory::IsBackendAvailable(CPUBackendType type)
{
    // Check if the backend is available (compiled in)
    switch (type)
    {
    case CPUBackendType::MAME:
#ifdef USE_MAME_BACKEND
        return true;
#else
        return false;
#endif
        
    case CPUBackendType::BOX86:
#ifdef USE_86BOX_BACKEND
        return true;
#else
        return false;
#endif
        
    case CPUBackendType::CUSTOM:
        return false; // Not implemented yet
        
    default:
        return false;
    }
}

} // namespace x86emu