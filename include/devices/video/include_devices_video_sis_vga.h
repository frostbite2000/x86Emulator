/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Copyright (C) 2011-2025 MAME development team (original SiS 630/730 implementation)
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

#ifndef X86EMULATOR_SIS_VGA_H
#define X86EMULATOR_SIS_VGA_H

#include "graphics_adapter.h"
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <array>

namespace x86emu {

// Forward declaration
class SiS630HostDevice;

/**
 * @brief SiS 630/730 VGA implementation
 * 
 * This class implements the SiS 630/730 VGA adapter based on the MAME implementation.
 */
class SisVgaAdapter : public GraphicsAdapter {
public:
    /**
     * @brief Construct a new Sis Vga Adapter
     */
    SisVgaAdapter();
    
    /**
     * @brief Destroy the Sis Vga Adapter
     */
    ~SisVgaAdapter() override;
    
    /**
     * @brief Initialize the SiS VGA adapter
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the SiS VGA adapter
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the SiS VGA adapter
     */
    void reset() override;
    
    /**
     * @brief Get the card type
     * 
     * @return Card type string
     */
    std::string getCardType() const override { return "SiS 630/730"; }
    
    /**
     * @brief Read from a VGA register
     * 
     * @param port Register port
     * @return Register value
     */
    uint8_t readRegister(uint16_t port) override;
    
    /**
     * @brief Write to a VGA register
     * 
     * @param port Register port
     * @param value Value to write
     */
    void writeRegister(uint16_t port, uint8_t value) override;
    
    /**
     * @brief Read from VGA memory
     * 
     * @param address Memory address
     * @return Memory value
     */
    uint8_t readMemory(uint32_t address) override;
    
    /**
     * @brief Write to VGA memory
     * 
     * @param address Memory address
     * @param value Value to write
     */
    void writeMemory(uint32_t address, uint8_t value) override;
    
    /**
     * @brief Read from the VGA ROM
     * 
     * @param address Memory address relative to ROM base
     * @return ROM value
     */
    uint8_t readRom(uint32_t address);
    
    /**
     * @brief Read from the VGA aperture (PCI BAR)
     * 
     * @param offset Offset from aperture base
     * @return Memory value
     */
    uint8_t readAperture(uint32_t offset);
    
    /**
     * @brief Write to the VGA aperture (PCI BAR)
     * 
     * @param offset Offset from aperture base
     * @param value Value to write
     */
    void writeAperture(uint32_t offset, uint8_t value);
    
    /**
     * @brief Set the host bridge
     * 
     * @param host Host bridge device
     */
    void setHostBridge(SiS630HostDevice* host);
    
    /**
     * @brief Set enabled state
     * 
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Set aperture base address
     * 
     * @param baseAddress Base address for the aperture
     */
    void setApertureBase(uint32_t baseAddress);
    
    /**
     * @brief Get enabled state
     * 
     * @return true if enabled
     * @return false if disabled
     */
    bool isEnabled() const { return m_enabled; }

private:
    // SiS VGA registers
    struct SisRegisters {
        // Standard VGA registers
        uint8_t misc_output;
        uint8_t sequencer_index;
        std::array<uint8_t, 0x08> sequencer_regs;
        uint8_t crtc_index;
        std::array<uint8_t, 0x19> crtc_regs;
        uint8_t graphics_index;
        std::array<uint8_t, 0x09> graphics_regs;
        uint8_t attribute_index;
        std::array<uint8_t, 0x15> attribute_regs;
        uint8_t attribute_state;
        uint8_t pel_index;
        uint8_t pel_index_read;
        uint8_t pel_index_write;
        std::array<uint8_t, 0x300> palette;
        
        // SiS-specific registers
        uint8_t sr_index;
        std::unordered_map<uint8_t, uint8_t> sr_regs;
        uint8_t cr_index;
        std::unordered_map<uint8_t, uint8_t> cr_regs;
        uint8_t gr_index;
        std::unordered_map<uint8_t, uint8_t> gr_regs;
        uint8_t ar_index;
        std::unordered_map<uint8_t, uint8_t> ar_regs;
        
        // SiS 630/730 specific registers
        uint8_t mmadr_index;
        std::array<uint8_t, 0x100> mmio_regs;
    };
    
    SisRegisters m_regs;
    
    // Video memory and framebuffer
    static constexpr uint32_t VRAM_SIZE = 16 * 1024 * 1024; // 16 MB VRAM
    std::vector<uint8_t> m_vram;
    std::vector<uint8_t> m_frameBuffer;
    
    // VGA BIOS ROM (64KB)
    static constexpr uint32_t ROM_SIZE = 64 * 1024;
    std::vector<uint8_t> m_rom;
    
    // Current video mode
    VideoMode m_mode;
    int m_width;
    int m_height;
    int m_bpp;
    
    // Host bridge connection
    SiS630HostDevice* m_hostBridge;
    
    // VGA aperture base address
    uint32_t m_apertureBase;
    
    // Enabled state
    bool m_enabled;
    
    // Helpers for register access
    uint8_t readVgaRegister(uint16_t port);
    void writeVgaRegister(uint16_t port, uint8_t value);
    uint8_t readSisRegister(uint16_t port);
    void writeSisRegister(uint16_t port, uint8_t value);
    uint8_t readMmioRegister(uint8_t index);
    void writeMmioRegister(uint8_t index, uint8_t value);
    
    // Helpers for rendering
    void updateFrameBuffer();
    void renderTextMode();
    void renderGraphicsMode();
    void calculateVideoMode();
    void updatePalette();
    
    // Character ROM for text mode
    std::vector<uint8_t> m_characterRom;
    
    // Memory access helpers
    void loadROM();
};

} // namespace x86emu

#endif // X86EMULATOR_SIS_VGA_H