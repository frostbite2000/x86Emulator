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

#ifndef X86EMULATOR_BIOS_LOADER_H
#define X86EMULATOR_BIOS_LOADER_H

#include "machine/machine_config.h"
#include "memory_manager.h"
#include <string>
#include <vector>

namespace x86emu {

/**
 * @brief BIOS ROM loader
 * 
 * This class handles loading of BIOS ROMs into memory.
 */
class BiosLoader {
public:
    /**
     * @brief Construct a new BiosLoader
     */
    BiosLoader() = default;
    
    /**
     * @brief Destroy the BiosLoader
     */
    ~BiosLoader() = default;
    
    /**
     * @brief Load BIOS ROM for a specific machine
     * 
     * @param machine Machine configuration
     * @param memory Memory manager
     * @return true if BIOS was loaded successfully
     * @return false if BIOS loading failed
     */
    bool loadBios(const std::shared_ptr<MachineConfig>& machine, MemoryManager* memory);
    
    /**
     * @brief Load BIOS ROM from a file
     * 
     * @param filename BIOS filename
     * @param memory Memory manager
     * @param address Memory address to load at
     * @param expectedSize Expected size of the BIOS file
     * @return true if BIOS was loaded successfully
     * @return false if BIOS loading failed
     */
    bool loadBiosFromFile(const std::string& filename, MemoryManager* memory, uint32_t address, uint32_t expectedSize);
    
    /**
     * @brief Generate a minimal placeholder BIOS
     * 
     * @param memory Memory manager
     * @param address Memory address to load at
     * @param size Size of the BIOS to generate
     * @return true if BIOS was generated successfully
     * @return false if BIOS generation failed
     */
    bool generatePlaceholderBios(MemoryManager* memory, uint32_t address, uint32_t size);
    
    /**
     * @brief Set the BIOS directory
     * 
     * @param directory Directory path
     */
    void setBiosDirectory(const std::string& directory) { m_biosDirectory = directory; }
    
    /**
     * @brief Get the BIOS directory
     * 
     * @return BIOS directory path
     */
    std::string getBiosDirectory() const { return m_biosDirectory; }
    
    /**
     * @brief Set placeholder BIOS for testing
     * 
     * @param usePlaceholder true to use placeholder BIOS
     */
    void setUsePlaceholderBios(bool usePlaceholder) { m_usePlaceholderBios = usePlaceholder; }
    
    /**
     * @brief Check if using placeholder BIOS
     * 
     * @return true if using placeholder BIOS
     * @return false if using real BIOS
     */
    bool isUsingPlaceholderBios() const { return m_usePlaceholderBios; }

private:
    std::string m_biosDirectory = "roms/bios";
    bool m_usePlaceholderBios = false;
    
    /**
     * @brief Create path for BIOS file
     * 
     * @param filename Filename
     * @return Full path to BIOS file
     */
    std::string createBiosPath(const std::string& filename) const;
    
    /**
     * @brief Initialize a placeholder BIOS with essential structures
     * 
     * @param data BIOS data
     * @param size BIOS size
     */
    void initPlaceholderBios(uint8_t* data, uint32_t size);
};

} // namespace x86emu

#endif // X86EMULATOR_BIOS_LOADER_H