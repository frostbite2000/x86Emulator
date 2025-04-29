#include "sis_630_vga.h"
#include "x86emulator/device_manager.h"
#include "x86emulator/chipsets/sis_630.h"
#include <algorithm>
#include <cassert>
#include <cstring>

// Include MAME SiS 630 VGA headers
#include "mame/src/devices/video/sis630_vga.h"
#include "mame/src/emu/emupal.h"

namespace x86emu {

SiS630VGA::SiS630VGA(const std::string& name, const std::string& chipsetName)
    : VideoDevice(name, "SiS 630 Integrated VGA")
    , m_chipsetName(chipsetName)
    , m_videoMode(VideoMode::TEXT)
    , m_width(80) // Default text mode
    , m_height(25)
    , m_bpp(4)
    , m_refreshRate(60)
    , m_inVSync(false)
    , m_videoMemorySize(32) // Default to 32MB
    , m_accelerationEnabled(true)
    , m_renderTarget(nullptr)
    , m_lastRenderTime(0)
    , m_frameskip(0)
    , m_frameCount(0)
    , m_dirtyRegion({0, 0, 0, 0})
{
    // Initialize palette with EGA/VGA default colors for text mode
    InitializeTextModePalette();
    
    // Initialize frame buffer with 8MB (enough for high resolutions)
    m_frameBuffer.resize(8 * 1024 * 1024);
    std::fill(m_frameBuffer.begin(), m_frameBuffer.end(), 0);
    
    // Initialize text buffer (80x25 characters with attributes)
    m_textBuffer.resize(80 * 25 * 2);
    std::fill(m_textBuffer.begin(), m_textBuffer.end(), 0);
    
    // Initialize font data with the default VGA 8x16 font
    m_fontData.resize(256 * 16); // 256 characters, 16 bytes per character
    InitializeDefaultFont();
    
    // Initialize hardware cursor data
    m_cursorEnabled = true;
    m_cursorPos = {0, 0};
    m_cursorStart = 14;  // Default cursor shape starts at scanline 14
    m_cursorEnd = 15;    // Default cursor shape ends at scanline 15
}

SiS630VGA::~SiS630VGA()
{
    // MAME device will be cleaned up by its unique_ptr destructor
}

bool SiS630VGA::Initialize()
{
    if (IsInitialized()) {
        return true;
    }
    
    try {
        // Look up the associated chipset device
        auto chipset = dynamic_cast<SiS630Chipset*>(DeviceManager::Instance().GetDevice(m_chipsetName));
        if (!chipset || !chipset->IsInitialized() || !chipset->IsIntegratedGraphicsEnabled()) {
            return false;
        }
        
        // Get the video memory size from the chipset
        m_videoMemorySize = chipset->GetIntegratedGraphicsMemory();
        
        // Create the MAME SiS 630 VGA device
        m_device = std::make_unique<mame::sis630_vga_device>(0);
        if (!m_device) {
            return false;
        }
        
        // Initialize the device
        m_device->device_start();
        
        // Reset the device
        Reset();
        
        // Initialize the acceleration engine
        InitializeAccelerationEngine();
        
        SetInitialized(true);
        return true;
    }
    catch (const std::exception& ex) {
        // Log the exception
        return false;
    }
}

void SiS630VGA::Reset()
{
    if (IsInitialized() && m_device) {
        // Reset the MAME device
        m_device->device_reset();
        
        // Reset to default text mode
        std::lock_guard<std::mutex> lock(m_accessMutex);
        m_videoMode = VideoMode::TEXT;
        m_width = 80;
        m_height = 25;
        m_bpp = 4;
        
        // Clear the frame and text buffers
        std::fill(m_frameBuffer.begin(), m_frameBuffer.end(), 0);
        std::fill(m_textBuffer.begin(), m_textBuffer.end(), 0);
        
        // Reset cursor state
        m_cursorPos = {0, 0};
        
        // Mark the entire screen as dirty
        m_dirtyRegion = {0, 0, m_width, m_height};
    }
}

void SiS630VGA::Update(uint32_t deltaTime)
{
    if (!IsInitialized()) {
        return;
    }
    
    m_lastRenderTime += deltaTime;
    
    // Update at ~60Hz (16.7ms)
    if (m_lastRenderTime >= 16667) {
        m_lastRenderTime = 0;
        
        // Skip frames if frameskip is enabled
        if (m_frameCount++ % (m_frameskip + 1) == 0) {
            // Update the frame buffer from the MAME device
            UpdateFrameBuffer();
        }
    }
}

VideoMode SiS630VGA::GetVideoMode() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_videoMode;
}

int SiS630VGA::GetWidth() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_width;
}

int SiS630VGA::GetHeight() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_height;
}

const uint8_t* SiS630VGA::GetFrameBuffer() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_frameBuffer.data();
}

size_t SiS630VGA::GetFrameBufferSize() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    if (m_videoMode == VideoMode::TEXT) {
        return m_width * m_height * 2; // 2 bytes per character (char + attribute)
    } else {
        return m_width * m_height * (m_bpp / 8);
    }
}

int SiS630VGA::GetBitsPerPixel() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_bpp;
}

int SiS630VGA::GetRefreshRate() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_refreshRate;
}

bool SiS630VGA::IsInVSync() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_inVSync;
}

void SiS630VGA::SetRenderTarget(void* target)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_renderTarget = target;
}

bool SiS630VGA::SetVideoMemorySize(uint32_t sizeMB)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Validate the size (SiS 630 supports 8MB to 64MB)
    if (sizeMB < 8 || sizeMB > 64) {
        return false;
    }
    
    // Update the video memory size
    m_videoMemorySize = sizeMB;
    
    // Update the chipset configuration
    auto chipset = dynamic_cast<SiS630Chipset*>(DeviceManager::Instance().GetDevice(m_chipsetName));
    if (chipset) {
        chipset->SetIntegratedGraphicsMemory(sizeMB);
    }
    
    return true;
}

uint32_t SiS630VGA::GetVideoMemorySize() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_videoMemorySize;
}

bool SiS630VGA::IsAccelerationEnabled() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_accelerationEnabled;
}

bool SiS630VGA::EnableAcceleration(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_accelerationEnabled = enabled;
    
    if (m_device) {
        // Configure acceleration in the MAME device
        // This is simplified - in a real implementation, this would interface with MAME
    }
    
    return true;
}

void SiS630VGA::SetFrameskip(int frameskip)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_frameskip = std::max(0, std::min(10, frameskip));
}

int SiS630VGA::GetFrameskip() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_frameskip;
}

void SiS630VGA::RenderToTarget(void* target, int targetWidth, int targetHeight)
{
    if (!target || !IsInitialized()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Check which rendering path to use
    if (m_videoMode == VideoMode::TEXT) {
        RenderTextMode(target, targetWidth, targetHeight);
    } else {
        RenderGraphicsMode(target, targetWidth, targetHeight);
    }
    
    // Clear the dirty region after rendering
    m_dirtyRegion = {0, 0, 0, 0};
}

void SiS630VGA::UpdateFrameBuffer()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    if (!m_device) {
        return;
    }
    
    // Check if the video mode has changed
    UpdateVideoMode();
    
    // Based on the current video mode, update the appropriate buffer
    if (m_videoMode == VideoMode::TEXT) {
        UpdateTextModeBuffer();
    } else {
        UpdateGraphicsModeBuffer();
    }
    
    // Update hardware cursor state
    UpdateCursor();
}

void SiS630VGA::UpdateVideoMode()
{
    if (!m_device) {
        return;
    }
    
    // In a real implementation, we would query the MAME device for mode changes
    // Here's a simplified version that checks MAME's register state
    
    // Access MAME's register state (simplified)
    bool isGraphicsMode = false;
    int width = 0;
    int height = 0;
    int bpp = 0;
    
    // For this example, we'll simulate a detection based on what would be in MAME's code
    
    // Check the Graphics Mode Register (typically at 0x3CE/0x06 in VGA)
    uint8_t graphicsModeReg = 0; // Would come from MAME
    
    // Check the Miscellaneous Output Register
    uint8_t miscOutputReg = 0; // Would come from MAME
    
    // Detect if we're in graphics mode based on register values
    isGraphicsMode = (graphicsModeReg & 0x01) != 0;
    
    if (isGraphicsMode) {
        // Determine resolution based on CRTC registers
        // These would be read from MAME's emulated CRTC
        uint8_t horizontalTotal = 0;       // 0x3D4/0x00
        uint8_t horizontalDisplay = 0;     // 0x3D4/0x01
        uint8_t verticalTotal = 0;         // 0x3D4/0x06
        uint8_t verticalDisplay = 0;       // 0x3D4/0x12
        
        // Calculate width and height
        // In actual implementation, these would be determined by reading from MAME
        width = 640;
        height = 480;
        bpp = 16;
        
        // Extended registers for higher resolutions
        // SiS 630 supports up to 1600x1200
        
        // Did mode change?
        if (m_videoMode != VideoMode::GRAPHICS || m_width != width || 
            m_height != height || m_bpp != bpp) {
            
            m_videoMode = VideoMode::GRAPHICS;
            m_width = width;
            m_height = height;
            m_bpp = bpp;
            
            // Resize frame buffer if needed
            size_t requiredSize = width * height * (bpp / 8);
            if (m_frameBuffer.size() < requiredSize) {
                m_frameBuffer.resize(requiredSize);
            }
            
            // Mark the whole screen as dirty
            m_dirtyRegion = {0, 0, width, height};
        }
    } else {
        // Text mode - Standard VGA text mode is 80x25
        width = 80;
        height = 25;
        bpp = 4;  // 4 bits per pixel for text mode
        
        if (m_videoMode != VideoMode::TEXT || m_width != width || 
            m_height != height || m_bpp != bpp) {
            
            m_videoMode = VideoMode::TEXT;
            m_width = width;
            m_height = height;
            m_bpp = bpp;
            
            // Resize text buffer if needed
            size_t requiredSize = width * height * 2; // 2 bytes per character
            if (m_textBuffer.size() < requiredSize) {
                m_textBuffer.resize(requiredSize);
            }
            
            // Mark the whole screen as dirty
            m_dirtyRegion = {0, 0, width, height};
        }
    }
}

void SiS630VGA::UpdateTextModeBuffer()
{
    if (!m_device) {
        return;
    }
    
    // In a real implementation, we would copy the text mode buffer from the MAME device
    // Here's a simplified version that would need to be adapted
    
    // For example, in MAME, the text buffer might be accessible like this:
    // uint16_t* textBuffer = m_device->get_text_buffer();
    
    // For now, we'll simulate text content
    // In text mode, we'd normally access the memory at 0xB8000 for color text mode
    
    // Simplified: Just update some sample text
    // In a real implementation, this would come from MAME's emulated memory
    
    // For demonstration, let's create some sample text
    const char* sampleText = "SiS 630 VGA Emulation - Text Mode";
    
    // Calculate text buffer offset
    int row = 0;
    int col = (m_width - strlen(sampleText)) / 2; // Center the text
    int offset = (row * m_width + col) * 2;
    
    // Copy text to buffer (char byte + attribute byte)
    for (size_t i = 0; i < strlen(sampleText); ++i) {
        m_textBuffer[offset + i*2] = sampleText[i];
        m_textBuffer[offset + i*2 + 1] = 0x07; // Light gray on black
    }
    
    // Mark this region as dirty
    m_dirtyRegion = {col, row, static_cast<int>(strlen(sampleText)), 1};
}

void SiS630VGA::UpdateGraphicsModeBuffer()
{
    if (!m_device) {
        return;
    }
    
    // In a real implementation, we would copy the frame buffer from the MAME device
    // This is how we would access the video memory in MAME
    
    // Get a pointer to MAME's video memory
    // uint8_t* mameVideoMem = m_device->get_vram_ptr();
    // size_t mameVideoMemSize = m_device->get_vram_size();
    
    // For our simplified example, we'll generate a test pattern
    GenerateTestPattern();
    
    // In a real implementation, we would need to handle different color depths
    // and maybe convert from MAME's internal format
    
    // Mark the entire screen as dirty
    m_dirtyRegion = {0, 0, m_width, m_height};
}

void SiS630VGA::UpdateCursor()
{
    if (!m_device) {
        return;
    }
    
    // In a real implementation, we would read cursor state from MAME
    // Here's a simplified version
    
    // For a full implementation, we'd read these values from MAME:
    // - Cursor position registers (0x3D4/0x0E and 0x3D4/0x0F)
    // - Cursor shape registers (0x3D4/0x0A and 0x3D4/0x0B)
    // - Cursor enable bit in the CRTC register
    
    // For now, just simulate a blinking cursor
    static int blinkCounter = 0;
    
    if (++blinkCounter >= 30) {
        m_cursorVisible = !m_cursorVisible;
        blinkCounter = 0;
        
        // Mark cursor area as dirty
        if (m_videoMode == VideoMode::TEXT) {
            m_dirtyRegion = {m_cursorPos.x, m_cursorPos.y, 1, 1};
        }
    }
}

void SiS630VGA::RenderTextMode(void* target, int targetWidth, int targetHeight)
{
    if (!target) {
        return;
    }
    
    // Cast the target to the appropriate type (assuming RGBA8888 pixel format)
    uint32_t* targetBuffer = static_cast<uint32_t*>(target);
    
    // Calculate scaling factors
    float scaleX = static_cast<float>(targetWidth) / (m_width * 8);  // 8 pixels per character
    float scaleY = static_cast<float>(targetHeight) / (m_height * 16); // 16 pixels per character
    
    // Calculate the dirty region in pixels
    int dirtyX = m_dirtyRegion.x * 8;
    int dirtyY = m_dirtyRegion.y * 16;
    int dirtyWidth = m_dirtyRegion.width * 8;
    int dirtyHeight = m_dirtyRegion.height * 16;
    
    // Clamp to screen boundaries
    dirtyWidth = std::min(dirtyWidth, m_width * 8);
    dirtyHeight = std::min(dirtyHeight, m_height * 16);
    
    // Render only the dirty region
    for (int y = dirtyY; y < dirtyY + dirtyHeight; ++y) {
        for (int x = dirtyX; x < dirtyX + dirtyWidth; ++x) {
            // Convert from character position to pixel position
            int charX = x / 8;
            int charY = y / 16;
            int charOffset = (charY * m_width + charX) * 2;
            
            // Get character and attribute
            uint8_t character = m_textBuffer[charOffset];
            uint8_t attribute = m_textBuffer[charOffset + 1];
            
            // Get foreground and background colors
            uint8_t fgColor = attribute & 0x0F;
            uint8_t bgColor = (attribute >> 4) & 0x07; // High bits can have special meaning
            
            // Calculate which pixel of the character we're rendering
            int fontX = x % 8;
            int fontY = y % 16;
            
            // Get the pixel from the font data
            uint8_t fontByte = m_fontData[character * 16 + fontY];
            bool pixelOn = (fontByte & (0x80 >> fontX)) != 0;
            
            // Check if we're rendering the cursor
            bool isCursor = m_cursorEnabled && m_cursorVisible && 
                           charX == m_cursorPos.x && charY == m_cursorPos.y &&
                           fontY >= m_cursorStart && fontY <= m_cursorEnd;
            
            // If this is the cursor position and it's visible, invert the pixel
            if (isCursor) {
                pixelOn = !pixelOn;
            }
            
            // Select the appropriate color
            uint32_t color = m_palette[pixelOn ? fgColor : bgColor];
            
            // Calculate the destination position in the target buffer
            int targetX = static_cast<int>(x * scaleX);
            int targetY = static_cast<int>(y * scaleY);
            
            // Draw scaled pixels
            int endX = static_cast<int>((x + 1) * scaleX);
            int endY = static_cast<int>((y + 1) * scaleY);
            
            for (int ty = targetY; ty < endY; ++ty) {
                for (int tx = targetX; tx < endX; ++tx) {
                    if (tx < targetWidth && ty < targetHeight) {
                        targetBuffer[ty * targetWidth + tx] = color;
                    }
                }
            }
        }
    }
}

void SiS630VGA::RenderGraphicsMode(void* target, int targetWidth, int targetHeight)
{
    if (!target) {
        return;
    }
    
    // Cast the target to the appropriate type (assuming RGBA8888 pixel format)
    uint32_t* targetBuffer = static_cast<uint32_t*>(target);
    
    // Calculate scaling factors
    float scaleX = static_cast<float>(targetWidth) / m_width;
    float scaleY = static_cast<float>(targetHeight) / m_height;
    
    // Calculate the dirty region in pixels
    int dirtyX = m_dirtyRegion.x;
    int dirtyY = m_dirtyRegion.y;
    int dirtyWidth = m_dirtyRegion.width;
    int dirtyHeight = m_dirtyRegion.height;
    
    // If no dirty region is specified, render the entire screen
    if (dirtyWidth <= 0 || dirtyHeight <= 0) {
        dirtyX = 0;
        dirtyY = 0;
        dirtyWidth = m_width;
        dirtyHeight = m_height;
    }
    
    // Clamp to screen boundaries
    dirtyWidth = std::min(dirtyWidth, m_width);
    dirtyHeight = std::min(dirtyHeight, m_height);
    
    // Render based on bit depth
    switch(m_bpp) {
        case 8: // 8-bit indexed color
            RenderGraphicsMode8bpp(targetBuffer, targetWidth, targetHeight,
                                  dirtyX, dirtyY, dirtyWidth, dirtyHeight,
                                  scaleX, scaleY);
            break;
            
        case 16: // 16-bit color (RGB565)
            RenderGraphicsMode16bpp(targetBuffer, targetWidth, targetHeight,
                                   dirtyX, dirtyY, dirtyWidth, dirtyHeight,
                                   scaleX, scaleY);
            break;
            
        case 24: // 24-bit color (RGB888)
        case 32: // 32-bit color (RGBA8888 or RGB888 + padding)
            RenderGraphicsMode32bpp(targetBuffer, targetWidth, targetHeight,
                                   dirtyX, dirtyY, dirtyWidth, dirtyHeight,
                                   scaleX, scaleY);
            break;
            
        default:
            // Unsupported bit depth
            break;
    }
}

void SiS630VGA::RenderGraphicsMode8bpp(uint32_t* targetBuffer, int targetWidth, int targetHeight,
                                      int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight,
                                      float scaleX, float scaleY)
{
    // Get pointer to the 8bpp frame buffer
    const uint8_t* srcBuffer = m_frameBuffer.data();
    int stride = m_width; // Number of bytes per row
    
    for (int y = dirtyY; y < dirtyY + dirtyHeight; ++y) {
        for (int x = dirtyX; x < dirtyX + dirtyWidth; ++x) {
            // Get the pixel value from the source buffer
            uint8_t paletteIndex = srcBuffer[y * stride + x];
            
            // Look up the color in the palette (assuming 256 entry palette)
            uint32_t color = m_palette[paletteIndex & 0xFF];
            
            // Calculate the destination position in the target buffer
            int targetX = static_cast<int>(x * scaleX);
            int targetY = static_cast<int>(y * scaleY);
            
            // Draw scaled pixels
            int endX = static_cast<int>((x + 1) * scaleX);
            int endY = static_cast<int>((y + 1) * scaleY);
            
            for (int ty = targetY; ty < endY; ++ty) {
                for (int tx = targetX; tx < endX; ++tx) {
                    if (tx < targetWidth && ty < targetHeight) {
                        targetBuffer[ty * targetWidth + tx] = color;
                    }
                }
            }
        }
    }
}

void SiS630VGA::RenderGraphicsMode16bpp(uint32_t* targetBuffer, int targetWidth, int targetHeight,
                                       int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight,
                                       float scaleX, float scaleY)
{
    // Get pointer to the 16bpp frame buffer
    const uint16_t* srcBuffer = reinterpret_cast<const uint16_t*>(m_frameBuffer.data());
    int stride = m_width; // Number of 16-bit values per row
    
    for (int y = dirtyY; y < dirtyY + dirtyHeight; ++y) {
        for (int x = dirtyX; x < dirtyX + dirtyWidth; ++x) {
            // Get the pixel value from the source buffer
            uint16_t pixel = srcBuffer[y * stride + x];
            
            // Convert RGB565 to RGBA8888
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
            
            // Calculate the destination position in the target buffer
            int targetX = static_cast<int>(x * scaleX);
            int targetY = static_cast<int>(y * scaleY);
            
            // Draw scaled pixels
            int endX = static_cast<int>((x + 1) * scaleX);
            int endY = static_cast<int>((y + 1) * scaleY);
            
            for (int ty = targetY; ty < endY; ++ty) {
                for (int tx = targetX; tx < endX; ++tx) {
                    if (tx < targetWidth && ty < targetHeight) {
                        targetBuffer[ty * targetWidth + tx] = color;
                    }
                }
            }
        }
    }
}

void SiS630VGA::RenderGraphicsMode32bpp(uint32_t* targetBuffer, int targetWidth, int targetHeight,
                                       int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight,
                                       float scaleX, float scaleY)
{
    // For 24bpp, we need to handle 3-byte pixels
    // For 32bpp, we can directly copy 4-byte pixels
    
    const uint8_t* srcBuffer = m_frameBuffer.data();
    int bytesPerPixel = m_bpp / 8;
    int stride = m_width * bytesPerPixel;
    
    for (int y = dirtyY; y < dirtyY + dirtyHeight; ++y) {
        for (int x = dirtyX; x < dirtyX + dirtyWidth; ++x) {
            // Get the pixel value from the source buffer
            uint32_t color;
            
            if (bytesPerPixel == 3) {
                // 24bpp: RGB order
                int offset = y * stride + x * 3;
                uint8_t r = srcBuffer[offset + 0];
                uint8_t g = srcBuffer[offset + 1];
                uint8_t b = srcBuffer[offset + 2];
                color = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else {
                // 32bpp: Either RGBA or RGBX
                int offset = y * stride + x * 4;
                uint8_t r = srcBuffer[offset + 0];
                uint8_t g = srcBuffer[offset + 1];
                uint8_t b = srcBuffer[offset + 2];
                uint8_t a = srcBuffer[offset + 3];
                color = (a << 24) | (r << 16) | (g << 8) | b;
            }
            
            // Calculate the destination position in the target buffer
            int targetX = static_cast<int>(x * scaleX);
            int targetY = static_cast<int>(y * scaleY);
            
            // Draw scaled pixels
            int endX = static_cast<int>((x + 1) * scaleX);
            int endY = static_cast<int>((y + 1) * scaleY);
            
            for (int ty = targetY; ty < endY; ++ty) {
                for (int tx = targetX; tx < endX; ++tx) {
                    if (tx < targetWidth && ty < targetHeight) {
                        targetBuffer[ty * targetWidth + tx] = color;
                    }
                }
            }
        }
    }
}

void SiS630VGA::GenerateTestPattern()
{
    // Create a test pattern to demonstrate graphics rendering
    // This would be replaced by actual frame data from the MAME device
    
    int bytesPerPixel = m_bpp / 8;
    int stride = m_width * bytesPerPixel;
    
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            // Create a gradient pattern
            uint8_t r = static_cast<uint8_t>((x * 255) / m_width);
            uint8_t g = static_cast<uint8_t>((y * 255) / m_height);
            uint8_t b = static_cast<uint8_t>(128 + (x + y) % 128);
            
            // Draw a white border
            if (x < 2 || y < 2 || x >= m_width - 2 || y >= m_height - 2) {
                r = g = b = 255;
            }
            
            // Draw some diagonal lines
            if ((x + y) % 32 < 2) {
                r = 255;
                g = b = 0;
            }
            
            // Write to the frame buffer based on bpp
            switch (m_bpp) {
                case 8: {
                    // 8-bit: use a grayscale value
                    uint8_t gray = static_cast<uint8_t>((r + g + b) / 3);
                    m_frameBuffer[y * m_width + x] = gray;
                    break;
                }
                    
                case 16: {
                    // 16-bit: RGB565
                    uint16_t color = ((r & 0xF8) << 8) | // 5 bits of red
                                     ((g & 0xFC) << 3) | // 6 bits of green
                                     ((b & 0xF8) >> 3);  // 5 bits of blue
                    
                    uint16_t* buffer16 = reinterpret_cast<uint16_t*>(m_frameBuffer.data());
                    buffer16[y * m_width + x] = color;
                    break;
                }
                    
                case 24: {
                    // 24-bit: RGB888
                    int offset = y * stride + x * 3;
                    m_frameBuffer[offset + 0] = r;
                    m_frameBuffer[offset + 1] = g;
                    m_frameBuffer[offset + 2] = b;
                    break;
                }
                    
                case 32: {
                    // 32-bit: RGBA8888
                    int offset = y * stride + x * 4;
                    m_frameBuffer[offset + 0] = r;
                    m_frameBuffer[offset + 1] = g;
                    m_frameBuffer[offset + 2] = b;
                    m_frameBuffer[offset + 3] = 255; // Alpha
                    break;
                }
            }
        }
    }
    
    // Draw SiS 630 text
    DrawTextInFrameBuffer("SiS 630 VGA", m_width / 2 - 50, m_height / 2, 0xFFFFFFFF);
    DrawTextInFrameBuffer(std::to_string(m_width) + "x" + std::to_string(m_height) + 
                         " " + std::to_string(m_bpp) + "bpp", 
                         m_width / 2 - 70, m_height / 2 + 20, 0xFFFFFFFF);
}

void SiS630VGA::DrawTextInFrameBuffer(const std::string& text, int x, int y, uint32_t color)
{
    // Simple text rendering function for the test pattern
    // This draws directly to the m_frameBuffer using the built-in font
    
    int bytesPerPixel = m_bpp / 8;
    int stride = m_width * bytesPerPixel;
    
    for (size_t i = 0; i < text.length(); ++i) {
        char ch = text[i];
        // Get the font data for this character
        const uint8_t* fontChar = &m_fontData[ch * 16];
        
        // Draw each pixel of the character
        for (int cy = 0; cy < 16; ++cy) {
            uint8_t fontRow = fontChar[cy];
            for (int cx = 0; cx < 8; ++cx) {
                if (fontRow & (0x80 >> cx)) {
                    int pixelX = x + i * 8 + cx;
                    int pixelY = y + cy;
                    
                    // Check if the pixel is within bounds
                    if (pixelX >= 0 && pixelX < m_width && pixelY >= 0 && pixelY < m_height) {
                        // Write the pixel based on bpp
                        switch (m_bpp) {
                            case 8: {
                                // 8-bit: use a grayscale value (white)
                                m_frameBuffer[pixelY * m_width + pixelX] = 255;
                                break;
                            }
                                
                            case 16: {
                                // 16-bit: RGB565
                                uint16_t color16 = 0xFFFF;  // White
                                uint16_t* buffer16 = reinterpret_cast<uint16_t*>(m_frameBuffer.data());
                                buffer16[pixelY * m_width + pixelX] = color16;
                                break;
                            }
                                
                            case 24: {
                                // 24-bit: RGB888
                                int offset = pixelY * stride + pixelX * 3;
                                m_frameBuffer[offset + 0] = (color >> 16) & 0xFF; // R
                                m_frameBuffer[offset + 1] = (color >> 8) & 0xFF;  // G
                                m_frameBuffer[offset + 2] = color & 0xFF;         // B
                                break;
                            }
                                
                            case 32: {
                                // 32-bit: RGBA8888
                                int offset = pixelY * stride + pixelX * 4;
                                m_frameBuffer[offset + 0] = (color >> 16) & 0xFF; // R
                                m_frameBuffer[offset + 1] = (color >> 8) & 0xFF;  // G
                                m_frameBuffer[offset + 2] = color & 0xFF;         // B
                                m_frameBuffer[offset + 3] = (color >> 24) & 0xFF; // A
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void SiS630VGA::InitializeTextModePalette()
{
    // Standard EGA/VGA 16-color palette
    m_palette[0]  = 0xFF000000; // Black
    m_palette[1]  = 0xFF0000AA; // Blue
    m_palette[2]  = 0xFF00AA00; // Green
    m_palette[3]  = 0xFF00AAAA; // Cyan
    m_palette[4]  = 0xFFAA0000; // Red
    m_palette[5]  = 0xFFAA00AA; // Magenta
    m_palette[6]  = 0xFFAA5500; // Brown
    m_palette[7]  = 0xFFAAAAAA; // Light Gray
    m_palette[8]  = 0xFF555555; // Dark Gray
    m_palette[9]  = 0xFF5555FF; // Light Blue
    m_palette[10] = 0xFF55FF55; // Light Green
    m_palette[11] = 0xFF55FFFF; // Light Cyan
    m_palette[12] = 0xFFFF5555; // Light Red
    m_palette[13] = 0xFFFF55FF; // Light Magenta
    m_palette[14] = 0xFFFFFF55; // Yellow
    m_palette[15] = 0xFFFFFFFF; // White
    
    // Fill the rest with black (for 256-color modes)
    for (int i = 16; i < 256; ++i) {
        m_palette[i] = 0xFF000000;
    }
}

void SiS630VGA::InitializeDefaultFont()
{
    // Load the default VGA 8x16 font
    // This is a simplified version with just a subset of characters
    
    // In a real implementation, we would load the font from a file or include
    // the complete VGA font data as a binary resource.
    
    // For demonstration purposes, we'll define a few basic characters
    
    // Define character 0 (space)
    std::fill(&m_fontData[0 * 16], &m_fontData[0 * 16 + 16], 0);
    
    // Example character: 'A' (ASCII 65)
    static const uint8_t charA[16] = {
        0b00000000,
        0b00000000,
        0b00011000,
        0b00111100,
        0b01100110,
        0b01100110,
        0b01111110,
        0b01100110,
        0b01100110,
        0b01100110,
        0b01100110,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };
    std::copy(charA, charA + 16, &m_fontData[65 * 16]);
    
    // ... similar definitions for other characters ...
    
    // For now, we'll just copy 'A' to other printable ASCII characters
    // to avoid defining them all
    for (int i = 32; i < 127; ++i) {
        if (i != 65) { // Skip 'A' which we already defined
            std::copy(charA, charA + 16, &m_fontData[i * 16]);
        }
    }
    
    // Define a basic box-drawing character for borders
    static const uint8_t charBox[16] = {
        0b11111111,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b11111111
    };
    std::copy(charBox, charBox + 16, &m_fontData[219 * 16]);
}

void SiS630VGA::InitializeAccelerationEngine()
{
    // In a real implementation, this would initialize the SiS 630 2D acceleration engine
    // SiS 630 supports hardware accelerated BitBlt, line drawing, color fill, etc.
    
    if (m_device) {
        // Configure acceleration parameters
        // This is simplified - in a real implementation, we would use MAME's API
        
        // Enable acceleration if configured
        // m_device->enable_acceleration(m_accelerationEnabled);
    }
}

} // namespace x86emu