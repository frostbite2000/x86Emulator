/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Portions Copyright (C) 2011-2025 MAME development team (original SiS 5513 implementation)
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

#ifndef X86EMULATOR_SIS5513_IDE_H
#define X86EMULATOR_SIS5513_IDE_H

#include "devices/pci/pci_device.h"
#include "devices/ata/ide_controller.h"
#include "memory_manager.h"
#include "io_manager.h"
#include <array>
#include <functional>

namespace x86emu {

/**
 * @brief SiS 5513 IDE Controller
 */
class SiS5513IdeDevice : public PCIDevice {
public:
    SiS5513IdeDevice();
    virtual ~SiS5513IdeDevice();

    bool initialize() override;
    void reset() override;

    uint32_t configRead(int function, int reg, uint32_t mem_mask = 0xFFFFFFFF) override;
    void configWrite(int function, int reg, uint32_t data, uint32_t mem_mask = 0xFFFFFFFF) override;

    void mapSpecialRegions(MemoryManager* memory_manager, IOManager* io_manager) override;
    
    /**
     * @brief Set IDE channel 1 (primary) IRQ callback
     */
    void setIRQPrimaryCallback(std::function<void(bool)> callback);
    
    /**
     * @brief Set IDE channel 2 (secondary) IRQ callback
     */
    void setIRQSecondaryCallback(std::function<void(bool)> callback);
    
    /**
     * @brief Get primary IDE controller
     */
    IdeController* getPrimaryController();
    
    /**
     * @brief Get secondary IDE controller
     */
    IdeController* getSecondaryController();

private:
    // IDE controllers
    std::unique_ptr<IdeController> m_ide1;  // Primary channel
    std::unique_ptr<IdeController> m_ide2;  // Secondary channel
    
    // IRQ callbacks
    std::function<void(bool)> m_irqPrimaryCallback;
    std::function<void(bool)> m_irqSecondaryCallback;
    
    // Special registers
    std::array<uint32_t, 5> m_bar;
    uint8_t m_ideCtrl0;
    uint8_t m_ideMisc;
    
    // Helper methods
    inline bool ide1Mode();
    inline bool ide2Mode();
    void flushIdeMode();
    
    // Register handlers
    uint32_t barRead(int offset);
    void barWrite(int offset, uint32_t data);
    void progIfWrite(uint8_t data);
    uint8_t ideCtrl0Read();
    void ideCtrl0Write(uint8_t data);
    uint8_t ideMiscCtrlRead();
    void ideMiscCtrlWrite(uint8_t data);
    
    // I/O port handlers
    uint32_t ide1ReadCs0(int offset);
    void ide1WriteCs0(int offset, uint32_t data);
    uint8_t ide1ReadCs1();
    void ide1WriteCs1(uint8_t data);
    uint32_t ide2ReadCs0(int offset);
    void ide2WriteCs0(int offset, uint32_t data);
    uint8_t ide2ReadCs1();
    void ide2WriteCs1(uint8_t data);
};

} // namespace x86emu

#endif // X86EMULATOR_SIS5513_IDE_H