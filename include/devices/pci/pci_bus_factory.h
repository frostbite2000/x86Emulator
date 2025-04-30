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

#ifndef X86EMULATOR_PCI_BUS_FACTORY_H
#define X86EMULATOR_PCI_BUS_FACTORY_H

#include "devices/pci/pci_bus.h"
#include <memory>

class ConfigManager;

namespace x86emu {

/**
 * @brief Supported PCI bus types
 */
enum class PCIBusType {
    SIS_630,    // SiS 630/730 chipset
    INTEL_440BX // Intel 440BX chipset
};

/**
 * @brief Factory for creating PCI bus configurations
 */
class PCIBusFactory {
public:
    /**
     * @brief Create a PCI bus with the specified type
     * 
     * @param type The type of PCI bus to create
     * @param ramSize The amount of RAM to allocate (in bytes)
     * @return A unique pointer to the created PCI bus, or nullptr on failure
     */
    static std::unique_ptr<PCIBus> createPCIBus(PCIBusType type, uint32_t ramSize);
    
    /**
     * @brief Get the PCI bus type from configuration
     * 
     * @param config The configuration manager
     * @return The PCI bus type specified in the configuration
     */
    static PCIBusType getPCIBusTypeFromConfig(ConfigManager* config);
};

} // namespace x86emu

#endif // X86EMULATOR_PCI_BUS_FACTORY_H