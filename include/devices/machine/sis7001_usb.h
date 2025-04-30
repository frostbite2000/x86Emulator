/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Portions Copyright (C) 2011-2025 MAME development team (original SiS 7001 implementation)
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

#ifndef X86EMULATOR_SIS7001_USB_H
#define X86EMULATOR_SIS7001_USB_H

#include "devices/pci/pci_device.h"
#include "memory_manager.h"
#include "io_manager.h"
#include "logger.h"

namespace x86emu {

/**
 * @brief SiS 7001 USB Host Controller device
 */
class SiS7001UsbDevice : public PCIDevice {
public:
    /**
     * @brief Construct a new SiS 7001 USB Controller device
     * 
     * @param numPorts Number of downstream USB ports
     */
    SiS7001UsbDevice(int numPorts = 2);
    
    /**
     * @brief Destroy the SiS 7001 USB Controller device
     */
    ~SiS7001UsbDevice() override;
    
    /**
     * @brief Initialize the USB controller
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Reset the USB controller
     */
    void reset() override;
    
    /**
     * @brief Map memory regions and I/O ports
     * 
     * @param memory_manager Memory manager instance
     * @param io_manager I/O manager instance
     */
    void mapSpecialRegions(MemoryManager* memory_manager, IOManager* io_manager) override;

private:
    // Registers and state
    uint32_t m_HcFmInterval;
    uint8_t m_downstreamPorts;
    
    // Memory region handlers
    void handleMemoryRead(uint32_t address, uint32_t* value);
    void handleMemoryWrite(uint32_t address, uint32_t value);
    
    // Base memory address
    uint32_t m_baseMemoryAddress;
};

} // namespace x86emu

#endif // X86EMULATOR_SIS7001_USB_H