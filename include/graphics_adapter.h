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

#ifndef X86EMULATOR_GRAPHICS_ADAPTER_H
#define X86EMULATOR_GRAPHICS_ADAPTER_H

#include "device.h"
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <QImage>

/**
 * @brief Graphics adapter for the emulator
 * 
 * Emulates a video card (VGA, EGA, etc.) with frame buffer access
 */
class GraphicsAdapter : public Device {
public:
    /**
     * @brief Video modes
     */
    enum class VideoMode {
        TEXT_40x25,    ///< Text mode 40x25
        TEXT_80x25,    ///< Text mode 80x25
        CGA_320x200,   ///< CGA 320x200x4
        CGA_640x200,   ///< CGA 640x200x2
        EGA_320x200,   ///< EGA 320x200x16
        EGA_640x200,   ///< EGA 640x200x16
        EGA_640x350,   ///< EGA 640x350x16
        VGA_320x200,   ///< VGA 320x200x256
        VGA_640x480,   ///< VGA 640x480x16
        SVGA_640x480,  ///< SVGA 640x480x256
        SVGA_800x600,  ///< SVGA 800x600x256
        SVGA_1024x768  ///< SVGA 1024x768x256
    };
    
    /**
     * @brief Construct a new Graphics Adapter
     * 
     * @param cardType Type of video card (e.g., "vga", "ega", "cga")
     */
    explicit GraphicsAdapter(const std::string& cardType);
    
    /**
     * @brief Destroy the Graphics Adapter
     */
    ~GraphicsAdapter() override;
    
    /**
     * @brief Initialize the graphics adapter
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the graphics adapter
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the graphics adapter
     */
    void reset() override;
    
    /**
     * @brief Get the current frame buffer
     * 
     * @param image Reference to QImage that will be filled with the frame buffer
     * @return true if the frame buffer was successfully retrieved
     * @return false if the frame buffer could not be retrieved
     */
    bool getFrameBuffer(QImage& image);
    
    /**
     * @brief Get the display width
     * 
     * @return Display width in pixels
     */
    int getDisplayWidth() const;
    
    /**
     * @brief Get the display height
     * 
     * @return Display height in pixels
     */
    int getDisplayHeight() const;
    
    /**
     * @brief Get the current video mode
     * 
     * @return Current video mode
     */
    VideoMode getVideoMode() const;
    
    /**
     * @brief Set the video mode
     * 
     * @param mode Video mode to set
     * @return true if the mode was set successfully
     * @return false if the mode could not be set
     */
    bool setVideoMode(VideoMode mode);
    
    /**
     * @brief Get the video card type
     * 
     * @return Video card type
     */
    std::string getCardType() const;
    
    /**
     * @brief Read from a video register
     * 
     * @param port Register port
     * @return Register value
     */
    uint8_t readRegister(uint16_t port) const;
    
    /**
     * @brief Write to a video register
     * 
     * @param port Register port
     * @param value Value to write
     */
    void writeRegister(uint16_t port, uint8_t value);
    
    /**
     * @brief Read from video memory
     * 
     * @param address Memory address
     * @return Memory value
     */
    uint8_t readMemory(uint32_t address) const;
    
    /**
     * @brief Write to video memory
     * 
     * @param address Memory address
     * @param value Value to write
     */
    void writeMemory(uint32_t address, uint8_t value);

private:
    // Video card type
    std::string m_cardType;
    
    // Current video mode
    VideoMode m_videoMode;
    
    // Frame buffer
    std::vector<uint8_t> m_frameBuffer;
    
    // Character ROM for text modes
    std::vector<uint8_t> m_characterROM;
    
    // Video memory
    std::vector<uint8_t> m_videoMemory;
    
    // Display dimensions
    int m_displayWidth;
    int m_displayHeight;
    
    // Color palette
    std::vector<uint32_t> m_palette;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;
    
    // VGA registers
    struct VGARegisters {
        uint8_t miscOutput;
        uint8_t sequencerIndex;
        std::array<uint8_t, 8> sequencerRegisters;
        uint8_t graphicsIndex;
        std::array<uint8_t, 16> graphicsRegisters;
        uint8_t attributeIndex;
        std::array<uint8_t, 32> attributeRegisters;
        uint8_t crtcIndex;
        std::array<uint8_t, 32> crtcRegisters;
        uint8_t dacState;
        uint8_t dacReadIndex;
        uint8_t dacWriteIndex;
        std::array<uint8_t, 768> dacData; // 256 colors * 3 components
    };
    
    VGARegisters m_registers;
    
    // Helper methods
    void updateFrameBuffer();
    void updateTextMode();
    void updateGraphicsMode();
    void updatePalette();
    void loadCharacterROM(const std::string& filename);
    bool isTextMode() const;
};

#endif // X86EMULATOR_GRAPHICS_ADAPTER_H