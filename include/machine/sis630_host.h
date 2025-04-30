/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Portions Copyright (C) 2011-2025 MAME development team (original SiS 630/730 implementation)
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

#ifndef X86EMULATOR_SIS630_HOST_H
#define X86EMULATOR_SIS630_HOST_H

#include "devices/pci/pci_host.h"
#include "memory_manager.h"
#include "io_manager.h"
#include "logger.h"
#include <array>

namespace x86emu {

/**
 * @brief SiS 630 Host controller device (Northbridge)
 */
class SiS630HostDevice : public PCIHostDevice {
public:
    SiS630HostDevice(uint32_t ram_size);
    virtual ~SiS630HostDevice();

    bool initialize() override;
    void reset() override;

    uint32_t configRead(int function, int reg, uint32_t mem_mask = 0xFFFFFFFF) override;
    void configWrite(int function, int reg, uint32_t data, uint32_t mem_mask = 0xFFFFFFFF) override;

    void mapSpecialRegions(MemoryManager* memory_manager, IOManager* io_manager) override;

    // PCI Functions
    enum Function {
        HOST_BRIDGE = 0,
        AGP_BRIDGE = 1
    };

private:
    // Memory window
    uint32_t m_ram_size;
    std::vector<uint32_t> m_ram;

    // Registers
    uint32_t m_shadow_ram_ctrl;
    uint8_t m_smram;
    uint8_t m_vga_control;
    uint8_t m_dram_status;
    uint8_t m_pci_hole;
    std::array<uint8_t, 12> m_agp_mailbox;
    uint32_t m_agp_priority_timer;

    struct {
        bool sba_enable;
        bool enable;
        uint8_t data_rate;
    } m_agp;

    // Helper functions
    void mapShadowRAM(MemoryManager* memory_manager, uint32_t start_offs, uint32_t end_offs, bool read_enable, bool write_enable);
    void updateMemoryMapping(MemoryManager* memory_manager);

    // Log for debug
    void logShadowMemory(uint32_t data);
    
    // Register handlers
    uint8_t readCapPtr();
    uint8_t readDramStatus();
    void writeDramStatus(uint8_t data);
    uint8_t readSmram();
    void writeSmram(uint8_t data);
    uint32_t readShadowRamCtrl();
    void writeShadowRamCtrl(uint32_t data);
    uint8_t readPCIHole();
    void writePCIHole(uint8_t data);
    uint8_t readVgaControl();
    void writeVgaControl(uint8_t data);
    
    // AGP interface (might move to separate class)
    uint32_t readAgpId();
    uint32_t readAgpStatus();
    uint32_t readAgpCommand();
    void writeAgpCommand(uint32_t data);
    uint32_t readAgpPriorityTimer();
    void writeAgpPriorityTimer(uint32_t data);
    uint8_t readAgpMailbox(int offset);
    void writeAgpMailbox(int offset, uint8_t data);
    
    // Logging helpers
    inline void logIO(const char* format, ...);
    inline void logMap(const char* format, ...);
    inline void logTodo(const char* format, ...);
    inline void logAgp(const char* format, ...);
};

} // namespace x86emu

#endif // X86EMULATOR_SIS630_HOST_H