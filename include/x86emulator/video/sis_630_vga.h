#pragma once

#include "x86emulator/video_device.h"
#include <array>
#include <mutex>
#include <vector>

// Forward declarations for MAME types
namespace mame {
class sis630_vga_device;
}

namespace x86emu {

/**
 * @brief SiS 630 integrated VGA implementation using MAME.
 * 
 * This class adapts MAME's SiS 630 VGA implementation to the x86Emulator
 * VideoDevice interface.
 */
class SiS630VGA : public VideoDevice {
public:
    /**
     * Create a SiS 630 VGA adapter.
     * @param name Unique device name
     * @param chipsetName Name of the chipset device this VGA is integrated with
     */
    SiS630VGA(const std::string& name, const std::string& chipsetName);
    
    /**
     * Destructor.
     */
    ~SiS630VGA() override;

    // Device interface implementation
    bool Initialize() override;
    void Reset() override;
    void Update(uint32_t deltaTime) override;
    
    // VideoDevice interface implementation
    VideoMode GetVideoMode() const override;
    int GetWidth() const override;
    int GetHeight() const override;
    const uint8_t* GetFrameBuffer() const override;
    size_t GetFrameBufferSize() const override;
    int GetBitsPerPixel() const override;
    int GetRefreshRate() const override;
    bool IsInVSync() const override;
    void SetRenderTarget(void* target) override;
    
    /**
     * Set the amount of video memory in MB.
     * @param sizeMB Memory size in MB
     * @return true if the operation was successful
     */
    bool SetVideoMemorySize(uint32_t sizeMB);
    
    /**
     * Get the amount of video memory in MB.
     * @return Memory size in MB
     */
    uint32_t GetVideoMemorySize() const;
    
    /**
     * Check if hardware acceleration is enabled.
     * @return true if hardware acceleration is enabled
     */
    bool IsAccelerationEnabled() const;
    
    /**
     * Enable or disable hardware acceleration.
     * @param enabled true to enable, false to disable
     * @return true if the operation was successful
     */
    bool EnableAcceleration(bool enabled);
    
    /**
     * Set the frameskip value (0 = no frameskip).
     * @param frameskip Number of frames to skip between renders
     */
    void SetFrameskip(int frameskip);
    
    /**
     * Get the current frameskip value.
     * @return Number of frames skipped between renders
     */
    int GetFrameskip() const;
    
    /**
     * Render the current frame buffer to a target buffer.
     * @param target Target buffer to render to
     * @param targetWidth Width of the target buffer
     * @param targetHeight Height of the target buffer
     */
    void RenderToTarget(void* target, int targetWidth, int targetHeight);
    
private:
    // MAME SiS 630 VGA device
    std::unique_ptr<mame::sis630_vga_device> m_device;
    
    // Associated chipset device name
    std::string m_chipsetName;
    
    // Frame buffer for graphics modes
    std::vector<uint8_t> m_frameBuffer;
    
    // Text buffer for text mode (character + attribute for each position)
    std::vector<uint8_t> m_textBuffer;
    
    // Font data (8x16 character cell)
    std::vector<uint8_t> m_fontData;
    
    // Color palette (256-entry RGBA)
    std::array<uint32_t, 256> m_palette;
    
    // Video mode parameters
    VideoMode m_videoMode;
    int m_width;
    int m_height;
    int m_bpp;
    int m_refreshRate;
    bool m_inVSync;
    
    // Video memory size in MB
    uint32_t m_videoMemorySize;
    
    // Hardware acceleration
    bool m_accelerationEnabled;
    
    // Render target
    void* m_renderTarget;
    
    // Performance settings
    uint32_t m_lastRenderTime;  // Microseconds since last render
    int m_frameskip;            // Number of frames to skip
    int m_frameCount;           // Counter for frameskip
    
    // Cursor state
    struct Point {
        int x, y;
    };
    Point m_cursorPos;
    bool m_cursorEnabled;
    bool m_cursorVisible;
    int m_cursorStart;
    int m_cursorEnd;
    
    // Dirty rectangle for optimized rendering
    struct Rect {
        int x, y, width, height;
    };
    Rect m_dirtyRegion;
    
    // Access synchronization
    mutable std::mutex m_accessMutex;
    
    // Helper methods
    
    // Update the frame buffer from MAME's VGA device
    void UpdateFrameBuffer();
    
    // Detect video mode changes
    void UpdateVideoMode();
    
    // Update text mode buffer
    void UpdateTextModeBuffer();
    
    // Update graphics mode buffer
    void UpdateGraphicsModeBuffer();
    
    // Update hardware cursor state
    void UpdateCursor();
    
    // Render text mode to a target buffer
    void RenderTextMode(void* target, int targetWidth, int targetHeight);
    
    // Render graphics mode to a target buffer
    void RenderGraphicsMode(void* target, int targetWidth, int targetHeight);
    
    // Rendering helpers for different bit depths
    void RenderGraphicsMode8bpp(uint32_t* target, int targetWidth, int targetHeight,
                              int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight,
                              float scaleX, float scaleY);
    
    void RenderGraphicsMode16bpp(uint32_t* target, int targetWidth, int targetHeight,
                               int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight,
                               float scaleX, float scaleY);
    
    void RenderGraphicsMode32bpp(uint32_t* target, int targetWidth, int targetHeight,
                               int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight,
                               float scaleX, float scaleY);
    
    // Generate a test pattern for graphics mode
    void GenerateTestPattern();
    
    // Draw text directly in the frame buffer (for test pattern)
    void DrawTextInFrameBuffer(const std::string& text, int x, int y, uint32_t color);
    
    // Initialize the text mode palette
    void InitializeTextModePalette();
    
    // Initialize the default VGA font
    void InitializeDefaultFont();
    
    // Initialize the SiS 630 acceleration engine
    void InitializeAccelerationEngine();
};

} // namespace x86emu