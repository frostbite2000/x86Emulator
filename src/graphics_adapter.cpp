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

#include "graphics_adapter.h"
#include "logger.h"

#include <algorithm>
#include <fstream>
#include <cstring>

// VGA I/O ports
constexpr uint16_t VGA_MISC_OUTPUT_READ = 0x3CC;
constexpr uint16_t VGA_MISC_OUTPUT_WRITE = 0x3C2;
constexpr uint16_t VGA_STATUS_REGISTER_0 = 0x3C2;
constexpr uint16_t VGA_STATUS_REGISTER_1 = 0x3DA;
constexpr uint16_t VGA_FEATURE_CONTROL_READ = 0x3CA;
constexpr uint16_t VGA_FEATURE_CONTROL_WRITE = 0x3BA;
constexpr uint16_t VGA_SEQUENCER_INDEX = 0x3C4;
constexpr uint16_t VGA_SEQUENCER_DATA = 0x3C5;
constexpr uint16_t VGA_GRAPHICS_INDEX = 0x3CE;
constexpr uint16_t VGA_GRAPHICS_DATA = 0x3CF;
constexpr uint16_t VGA_CRTC_INDEX = 0x3D4;
constexpr uint16_t VGA_CRTC_DATA = 0x3D5;
constexpr uint16_t VGA_ATTRIBUTE_INDEX = 0x3C0;
constexpr uint16_t VGA_ATTRIBUTE_DATA = 0x3C1;
constexpr uint16_t VGA_DAC_READ_INDEX = 0x3C7;
constexpr uint16_t VGA_DAC_WRITE_INDEX = 0x3C8;
constexpr uint16_t VGA_DAC_DATA = 0x3C9;

GraphicsAdapter::GraphicsAdapter(const std::string& cardType)
    : Device("GraphicsAdapter_" + cardType),
      m_cardType(cardType),
      m_videoMode(VideoMode::TEXT_80x25),
      m_displayWidth(720),
      m_displayHeight(400)
{
}

GraphicsAdapter::~GraphicsAdapter()
{
}

bool GraphicsAdapter::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Initialize video memory
        m_videoMemory.resize(256 * 1024, 0); // 256 KB video memory for VGA
        
        // Initialize frame buffer
        m_frameBuffer.resize(m_displayWidth * m_displayHeight * 4, 0); // RGBA format
        
        // Initialize palette
        m_palette.resize(256, 0);
        updatePalette();
        
        // Load character ROM
        loadCharacterROM("roms/char.rom");
        
        // Initialize registers
        std::memset(&m_registers, 0, sizeof(m_registers));
        
        // Set default register values for text mode
        if (m_cardType == "vga") {
            // VGA 80x25 text mode defaults
            m_registers.miscOutput = 0x67;
            m_registers.sequencerRegisters[0] = 0x03; // Reset and clock registers
            m_registers.sequencerRegisters[1] = 0x00; // Clocking mode
            m_registers.sequencerRegisters[2] = 0x03; // Map mask
            m_registers.sequencerRegisters[3] = 0x00; // Character map select
            m_registers.sequencerRegisters[4] = 0x02; // Memory mode
            
            m_registers.graphicsRegisters[0] = 0x00; // Set/reset
            m_registers.graphicsRegisters[1] = 0x00; // Enable set/reset
            m_registers.graphicsRegisters[2] = 0x00; // Color compare
            m_registers.graphicsRegisters[3] = 0x00; // Data rotate
            m_registers.graphicsRegisters[4] = 0x00; // Read map select
            m_registers.graphicsRegisters[5] = 0x10; // Mode
            m_registers.graphicsRegisters[6] = 0x0E; // Miscellaneous
            m_registers.graphicsRegisters[7] = 0x00; // Color don't care
            m_registers.graphicsRegisters[8] = 0xFF; // Bit mask
            
            // Default CRT controller registers for 80x25 text mode
            m_registers.crtcRegisters[0] = 0x5F; // Horizontal total
            m_registers.crtcRegisters[1] = 0x4F; // Horizontal displayed
            m_registers.crtcRegisters[2] = 0x50; // Horizontal sync position
            m_registers.crtcRegisters[3] = 0x82; // Horizontal sync width
            m_registers.crtcRegisters[4] = 0x55; // Vertical total
            m_registers.crtcRegisters[5] = 0x81; // Vertical total adjust
            m_registers.crtcRegisters[6] = 0xBF; // Vertical displayed
            m_registers.crtcRegisters[7] = 0x1F; // Vertical sync position
            m_registers.crtcRegisters[8] = 0x00; // Interlace mode
            m_registers.crtcRegisters[9] = 0x4F; // Maximum scan line
            m_registers.crtcRegisters[10] = 0x0E; // Cursor start
            m_registers.crtcRegisters[11] = 0x0F; // Cursor end
            m_registers.crtcRegisters[12] = 0x00; // Start address high
            m_registers.crtcRegisters[13] = 0x00; // Start address low
            m_registers.crtcRegisters[14] = 0x00; // Cursor location high
            m_registers.crtcRegisters[15] = 0x00; // Cursor location low
            
            // VGA attribute registers
            for (int i = 0; i < 16; i++) {
                m_registers.attributeRegisters[i] = i; // Palette registers
            }
            m_registers.attributeRegisters[16] = 0x01; // Mode control
            m_registers.attributeRegisters[17] = 0x00; // Overscan color
            m_registers.attributeRegisters[18] = 0x0F; // Color plane enable
            m_registers.attributeRegisters[19] = 0x00; // Horizontal pixel panning
            m_registers.attributeRegisters[20] = 0x00; // Color select
        }
        
        // Set initial video mode
        setVideoMode(VideoMode::TEXT_80x25);
        
        Logger::GetInstance()->info("Graphics adapter initialized: %s", m_cardType.c_str());
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Graphics adapter initialization failed: %s", ex.what());
        return false;
    }
}

void GraphicsAdapter::update(int cycles)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update the frame buffer based on the current video memory and mode
    updateFrameBuffer();
}

void GraphicsAdapter::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset video memory to zeros
    std::fill(m_videoMemory.begin(), m_videoMemory.end(), 0);
    
    // Reset frame buffer
    std::fill(m_frameBuffer.begin(), m_frameBuffer.end(), 0);
    
    // Reset registers
    std::memset(&m_registers, 0, sizeof(m_registers));
    
    // Set initial video mode
    setVideoMode(VideoMode::TEXT_80x25);
    
    Logger::GetInstance()->info("Graphics adapter reset");
}

bool GraphicsAdapter::getFrameBuffer(QImage& image)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Create image from frame buffer
        image = QImage(m_frameBuffer.data(), m_displayWidth, m_displayHeight, m_displayWidth * 4, QImage::Format_RGBA8888);
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Failed to get frame buffer: %s", ex.what());
        return false;
    }
}

int GraphicsAdapter::getDisplayWidth() const
{
    return m_displayWidth;
}

int GraphicsAdapter::getDisplayHeight() const
{
    return m_displayHeight;
}

GraphicsAdapter::VideoMode GraphicsAdapter::getVideoMode() const
{
    return m_videoMode;
}

bool GraphicsAdapter::setVideoMode(VideoMode mode)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Save current mode
    VideoMode oldMode = m_videoMode;
    
    // Set new mode
    m_videoMode = mode;
    
    // Update display dimensions based on mode
    switch (mode) {
        case VideoMode::TEXT_40x25:
            m_displayWidth = 360;
            m_displayHeight = 400;
            break;
        case VideoMode::TEXT_80x25:
            m_displayWidth = 720;
            m_displayHeight = 400;
            break;
        case VideoMode::CGA_320x200:
            m_displayWidth = 320;
            m_displayHeight = 200;
            break;
        case VideoMode::CGA_640x200:
            m_displayWidth = 640;
            m_displayHeight = 200;
            break;
        case VideoMode::EGA_320x200:
            m_displayWidth = 320;
            m_displayHeight = 200;
            break;
        case VideoMode::EGA_640x200:
            m_displayWidth = 640;
            m_displayHeight = 200;
            break;
        case VideoMode::EGA_640x350:
            m_displayWidth = 640;
            m_displayHeight = 350;
            break;
        case VideoMode::VGA_320x200:
            m_displayWidth = 320;
            m_displayHeight = 200;
            break;
        case VideoMode::VGA_640x480:
            m_displayWidth = 640;
            m_displayHeight = 480;
            break;
        case VideoMode::SVGA_640x480:
            m_displayWidth = 640;
            m_displayHeight = 480;
            break;
        case VideoMode::SVGA_800x600:
            m_displayWidth = 800;
            m_displayHeight = 600;
            break;
        case VideoMode::SVGA_1024x768:
            m_displayWidth = 1024;
            m_displayHeight = 768;
            break;
    }
    
    // Resize frame buffer if needed
    m_frameBuffer.resize(m_displayWidth * m_displayHeight * 4, 0);
    
    // Update display
    updateFrameBuffer();
    
    Logger::GetInstance()->info("Set video mode to %s (%dx%d)",
                              isTextMode() ? "text" : "graphics",
                              m_displayWidth, m_displayHeight);
    
    return true;
}

std::string GraphicsAdapter::getCardType() const
{
    return m_cardType;
}

uint8_t GraphicsAdapter::readRegister(uint16_t port) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Handle register reads
    switch (port) {
        case VGA_MISC_OUTPUT_READ:
            return m_registers.miscOutput;
        case VGA_SEQUENCER_INDEX:
            return m_registers.sequencerIndex;
        case VGA_SEQUENCER_DATA:
            if (m_registers.sequencerIndex < 8) {
                return m_registers.sequencerRegisters[m_registers.sequencerIndex];
            }
            break;
        case VGA_GRAPHICS_INDEX:
            return m_registers.graphicsIndex;
        case VGA_GRAPHICS_DATA:
            if (m_registers.graphicsIndex < 16) {
                return m_registers.graphicsRegisters[m_registers.graphicsIndex];
            }
            break;
        case VGA_CRTC_INDEX:
            return m_registers.crtcIndex;
        case VGA_CRTC_DATA:
            if (m_registers.crtcIndex < 32) {
                return m_registers.crtcRegisters[m_registers.crtcIndex];
            }
            break;
        case VGA_ATTRIBUTE_DATA:
            if (m_registers.attributeIndex < 32) {
                return m_registers.attributeRegisters[m_registers.attributeIndex];
            }
            break;
        case VGA_DAC_READ_INDEX:
            return m_registers.dacReadIndex;
        case VGA_DAC_WRITE_INDEX:
            return m_registers.dacWriteIndex;
        case VGA_DAC_DATA:
            if (m_registers.dacReadIndex < 256) {
                uint8_t value = m_registers.dacData[m_registers.dacReadIndex * 3 + m_registers.dacState];
                m_registers.dacState = (m_registers.dacState + 1) % 3;
                if (m_registers.dacState == 0) {
                    m_registers.dacReadIndex++;
                }
                return value;
            }
            break;
        case VGA_STATUS_REGISTER_1:
            // Return vertical retrace and display enable status
            // Bit 0: Display Enable (1 = display off, 0 = display on)
            // Bit 3: Vertical Retrace (1 = in retrace, 0 = not in retrace)
            // We simulate this with a toggling value
            return (rand() % 2) << 3; // Randomly toggle vertical retrace bit
    }
    
    Logger::GetInstance()->debug("Read from unknown VGA register: 0x%04X", port);
    return 0xFF;
}

void GraphicsAdapter::writeRegister(uint16_t port, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Handle register writes
    switch (port) {
        case VGA_MISC_OUTPUT_WRITE:
            m_registers.miscOutput = value;
            break;
        case VGA_SEQUENCER_INDEX:
            m_registers.sequencerIndex = value & 0x07;
            break;
        case VGA_SEQUENCER_DATA:
            if (m_registers.sequencerIndex < 8) {
                m_registers.sequencerRegisters[m_registers.sequencerIndex] = value;
            }
            break;
        case VGA_GRAPHICS_INDEX:
            m_registers.graphicsIndex = value & 0x0F;
            break;
        case VGA_GRAPHICS_DATA:
            if (m_registers.graphicsIndex < 16) {
                m_registers.graphicsRegisters[m_registers.graphicsIndex] = value;
            }
            break;
        case VGA_CRTC_INDEX:
            m_registers.crtcIndex = value & 0x1F;
            break;
        case VGA_CRTC_DATA:
            if (m_registers.crtcIndex < 32) {
                m_registers.crtcRegisters[m_registers.crtcIndex] = value;
            }
            break;
        case VGA_ATTRIBUTE_INDEX:
            m_registers.attributeIndex = value & 0x1F;
            break;
        case VGA_DAC_READ_INDEX:
            m_registers.dacReadIndex = value;
            m_registers.dacState = 0;
            break;
        case VGA_DAC_WRITE_INDEX:
            m_registers.dacWriteIndex = value;
            m_registers.dacState = 0;
            break;
        case VGA_DAC_DATA:
            if (m_registers.dacWriteIndex < 256) {
                m_registers.dacData[m_registers.dacWriteIndex * 3 + m_registers.dacState] = value;
                m_registers.dacState = (m_registers.dacState + 1) % 3;
                if (m_registers.dacState == 0) {
                    // All three components written, update palette and increment index
                    updatePalette();
                    m_registers.dacWriteIndex++;
                }
            }
            break;
        default:
            Logger::GetInstance()->debug("Write to unknown VGA register: 0x%04X = 0x%02X", port, value);
            break;
    }
}

uint8_t GraphicsAdapter::readMemory(uint32_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is within video memory
    if (address < m_videoMemory.size()) {
        return m_videoMemory[address];
    }
    
    return 0xFF;
}

void GraphicsAdapter::writeMemory(uint32_t address, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if address is within video memory
    if (address < m_videoMemory.size()) {
        m_videoMemory[address] = value;
    }
}

void GraphicsAdapter::updateFrameBuffer()
{
    // Update frame buffer based on current video mode
    if (isTextMode()) {
        updateTextMode();
    } else {
        updateGraphicsMode();
    }
}

void GraphicsAdapter::updateTextMode()
{
    // TODO: Implement text mode rendering
    // For now, just fill with a gradient pattern
    
    const int charWidth = m_displayWidth / 80;
    const int charHeight = m_displayHeight / 25;
    
    for (int y = 0; y < m_displayHeight; y++) {
        for (int x = 0; x < m_displayWidth; x++) {
            // Calculate pixel position in frame buffer (RGBA format)
            int pixelIndex = (y * m_displayWidth + x) * 4;
            
            // Calculate character position
            int charX = x / charWidth;
            int charY = y / charHeight;
            int charIndex = charY * 80 + charX;
            
            // Get character from video memory (even addresses are ASCII, odd addresses are attributes)
            uint8_t ascii = (charIndex < 2000) ? m_videoMemory[charIndex * 2] : 0;
            uint8_t attr = (charIndex < 2000) ? m_videoMemory[charIndex * 2 + 1] : 0;
            
            // Extract foreground and background colors
            uint8_t fgColor = attr & 0x0F;
            uint8_t bgColor = (attr >> 4) & 0x07;
            
            // Get colors from palette
            uint32_t fg = m_palette[fgColor];
            uint32_t bg = m_palette[bgColor];
            
            // Calculate position within character cell
            int cellX = x % charWidth;
            int cellY = y % charHeight;
            
            // Get character bitmap
            bool pixelOn = false;
            if (!m_characterROM.empty() && ascii < 256) {
                int romOffset = ascii * 16 + cellY;
                if (romOffset < static_cast<int>(m_characterROM.size())) {
                    uint8_t charRow = m_characterROM[romOffset];
                    pixelOn = (charRow & (0x80 >> cellX)) != 0;
                }
            }
            
            // Set pixel color
            uint32_t color = pixelOn ? fg : bg;
            
            // Write to frame buffer (RGBA format)
            m_frameBuffer[pixelIndex + 0] = (color >> 16) & 0xFF; // R
            m_frameBuffer[pixelIndex + 1] = (color >> 8) & 0xFF;  // G
            m_frameBuffer[pixelIndex + 2] = color & 0xFF;         // B
            m_frameBuffer[pixelIndex + 3] = 0xFF;                 // A (always fully opaque)
        }
    }
}

void GraphicsAdapter::updateGraphicsMode()
{
    // TODO: Implement graphics mode rendering
    // For now, just fill with a simple pattern
    
    switch (m_videoMode) {
        case VideoMode::VGA_320x200: {
            // Mode 13h - 320x200x256 (linear)
            for (int y = 0; y < m_displayHeight; y++) {
                for (int x = 0; x < m_displayWidth; x++) {
                    // Calculate pixel position in frame buffer (RGBA format)
                    int pixelIndex = (y * m_displayWidth + x) * 4;
                    
                    // Calculate pixel position in video memory
                    int memIndex = y * m_displayWidth + x;
                    
                    // Get pixel color from video memory
                    uint8_t colorIndex = (memIndex < 64000) ? m_videoMemory[memIndex] : 0;
                    
                    // Get color from palette
                    uint32_t color = m_palette[colorIndex];
                    
                    // Write to frame buffer (RGBA format)
                    m_frameBuffer[pixelIndex + 0] = (color >> 16) & 0xFF; // R
                    m_frameBuffer[pixelIndex + 1] = (color >> 8) & 0xFF;  // G
                    m_frameBuffer[pixelIndex + 2] = color & 0xFF;         // B
                    m_frameBuffer[pixelIndex + 3] = 0xFF;                 // A (always fully opaque)
                }
            }
            break;
        }
        
        default: {
            // For other modes, just show a simple pattern
            for (int y = 0; y < m_displayHeight; y++) {
                for (int x = 0; x < m_displayWidth; x++) {
                    // Calculate pixel position in frame buffer (RGBA format)
                    int pixelIndex = (y * m_displayWidth + x) * 4;
                    
                    // Create a simple gradient pattern
                    uint8_t r = static_cast<uint8_t>((x * 255) / m_displayWidth);
                    uint8_t g = static_cast<uint8_t>((y * 255) / m_displayHeight);
                    uint8_t b = 128;
                    
                    // Write to frame buffer (RGBA format)
                    m_frameBuffer[pixelIndex + 0] = r;
                    m_frameBuffer[pixelIndex + 1] = g;
                    m_frameBuffer[pixelIndex + 2] = b;
                    m_frameBuffer[pixelIndex + 3] = 0xFF;
                }
            }
            break;
        }
    }
}

void GraphicsAdapter::updatePalette()
{
    // Update palette from DAC registers
    for (int i = 0; i < 256; i++) {
        uint8_t r = m_registers.dacData[i * 3 + 0];
        uint8_t g = m_registers.dacData[i * 3 + 1];
        uint8_t b = m_registers.dacData[i * 3 + 2];
        
        // Convert 6-bit DAC values (0-63) to 8-bit RGB (0-255)
        r = (r << 2) | (r >> 4);
        g = (g << 2) | (g >> 4);
        b = (b << 2) | (b >> 4);
        
        // Store as RGBA (MSB = R, LSB = B)
        m_palette[i] = (r << 16) | (g << 8) | b;
    }
    
    // For text modes, set default CGA 16-color palette if palette is empty
    if (isTextMode() && std::all_of(m_palette.begin(), m_palette.begin() + 16, [](uint32_t c) { return c == 0; })) {
        // CGA 16-color palette (RGBA format)
        static const uint32_t cgaPalette[16] = {
            0x000000, // 0: Black
            0x0000AA, // 1: Blue
            0x00AA00, // 2: Green
            0x00AAAA, // 3: Cyan
            0xAA0000, // 4: Red
            0xAA00AA, // 5: Magenta
            0xAA5500, // 6: Brown
            0xAAAAAA, // 7: Light Gray
            0x555555, // 8: Dark Gray
            0x5555FF, // 9: Light Blue
            0x55FF55, // 10: Light Green
            0x55FFFF, // 11: Light Cyan
            0xFF5555, // 12: Light Red
            0xFF55FF, // 13: Light Magenta
            0xFFFF55, // 14: Yellow
            0xFFFFFF  // 15: White
        };
        
        // Copy palette
        std::copy(std::begin(cgaPalette), std::end(cgaPalette), m_palette.begin());
    }
}

void GraphicsAdapter::loadCharacterROM(const std::string& filename)
{
    try {
        // Open character ROM file
        std::ifstream file(filename, std::ios::binary);
        if (file) {
            // Get file size
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            // Read file into character ROM
            m_characterROM.resize(size);
            file.read(reinterpret_cast<char*>(m_characterROM.data()), size);
            
            Logger::GetInstance()->info("Loaded character ROM: %s (%zu bytes)", filename.c_str(), size);
        } else {
            Logger::GetInstance()->warn("Failed to open character ROM: %s", filename.c_str());
            
            // Create a basic 8x16 font as fallback
            m_characterROM.resize(256 * 16, 0);
            
            // Very simple font definition for a few basic characters
            // Space
            // Underscore
            m_characterROM[('_' * 16) + 15] = 0xFF;
            // Hash
            for (int i = 4; i < 12; i += 4) {
                m_characterROM[('#' * 16) + i] = 0xFF;
                m_characterROM[('#' * 16) + i + 1] = 0xFF;
            }
            for (int i = 0; i < 16; i++) {
                if (i != 4 && i != 5 && i != 8 && i != 9) {
                    m_characterROM[('#' * 16) + i] |= 0x24;
                }
            }
            // Asterisk
            m_characterROM[('*' * 16) + 4] = 0x10;
            m_characterROM[('*' * 16) + 5] = 0x54;
            m_characterROM[('*' * 16) + 6] = 0x38;
            m_characterROM[('*' * 16) + 7] = 0x54;
            m_characterROM[('*' * 16) + 8] = 0x10;
        }
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Error loading character ROM: %s", ex.what());
    }
}

bool GraphicsAdapter::isTextMode() const
{
    return m_videoMode == VideoMode::TEXT_40x25 || m_videoMode == VideoMode::TEXT_80x25;
}