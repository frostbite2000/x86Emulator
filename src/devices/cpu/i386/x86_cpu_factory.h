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

#ifndef X86EMULATOR_X86_CPU_FACTORY_H
#define X86EMULATOR_X86_CPU_FACTORY_H

#include "common/i386_interface.h"
#include <memory>
#include <string>

namespace x86emu {

/**
 * @brief CPU backend types
 */
enum class CPUBackendType {
    MAME,   // MAME CPU backend
    BOX86,  // 86Box CPU backend
    CUSTOM  // Your custom implementation
};

/**
 * @brief Factory for creating CPU instances
 */
class X86CPUFactory {
public:
    /**
     * @brief Create a CPU instance of the specified type
     * 
     * @param type The backend type to create
     * @param cpuModel The specific CPU model (e.g., "i386", "i486", "pentium")
     * @return std::unique_ptr<I386CPUInterface> A unique pointer to the created CPU instance
     */
    static std::unique_ptr<I386CPUInterface> CreateCPU(CPUBackendType type, const std::string& cpuModel);
    
    /**
     * @brief Get the default CPU backend type
     * 
     * @return CPUBackendType The default backend type
     */
    static CPUBackendType GetDefaultBackendType();
    
    /**
     * @brief Check if a CPU backend type is available
     * 
     * @param type The backend type to check
     * @return bool True if the backend is available, false otherwise
     */
    static bool IsBackendAvailable(CPUBackendType type);
};

} // namespace x86emu

#endif // X86EMULATOR_X86_CPU_FACTORY_H