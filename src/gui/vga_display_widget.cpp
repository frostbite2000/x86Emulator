#include "x86emulator/gui/vga_display_widget.h"
#include "x86emulator/video_device.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

namespace x86emu {

VGADisplayWidget::VGADisplayWidget(QWidget *parent)
    : QWidget(parent)
    , m_videoDevice(nullptr)
    , m_refreshRate(60)
    , m_preserveAspectRatio(true)
{
    // Set background color
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
    
    // Create initial display image
    createDisplayImage();
    
    // Set up refresh timer
    connect(&m_refreshTimer, &QTimer::timeout, this, &VGADisplayWidget::updateDisplay);
    m_refreshTimer.start(1000 / m_refreshRate);
    
    // Enable mouse tracking for cursor handling
    setMouseTracking(true);
}

VGADisplayWidget::~VGADisplayWidget()
{
    m_refreshTimer.stop();
}

void VGADisplayWidget::SetVideoDevice(VideoDevice* device)
{
    std::lock_guard<std::mutex> lock(m_imageMutex);
    m_videoDevice = device;
    
    // Update the display image size
    createDisplayImage();
}

void VGADisplayWidget::SetRefreshRate(int fps)
{
    if (fps < 1) fps = 1;
    if (fps > 240) fps = 240;
    
    m_refreshRate = fps;
    m_refreshTimer.setInterval(1000 / fps);
}

int VGADisplayWidget::GetRefreshRate() const
{
    return m_refreshRate;
}

void VGADisplayWidget::PreserveAspectRatio(bool preserve)
{
    m_preserveAspectRatio = preserve;
    update(); // Trigger repaint
}

bool VGADisplayWidget::IsAspectRatioPreserved() const
{
    return m_preserveAspectRatio;
}

void VGADisplayWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    
    // Calculate the display rectangle
    QRect displayRect;
    
    if (m_preserveAspectRatio && !m_displayImage.isNull()) {
        // Preserve aspect ratio
        float imageAspect = static_cast<float>(m_displayImage.width()) / m_displayImage.height();
        float widgetAspect = static_cast<float>(width()) / height();
        
        if (imageAspect > widgetAspect) {
            // Width is the limiting factor
            int displayHeight = static_cast<int>(width() / imageAspect);
            int yOffset = (height() - displayHeight) / 2;
            displayRect = QRect(0, yOffset, width(), displayHeight);
        } else {
            // Height is the limiting factor
            int displayWidth = static_cast<int>(height() * imageAspect);
            int xOffset = (width() - displayWidth) / 2;
            displayRect = QRect(xOffset, 0, displayWidth, height());
        }
    } else {
        // Fill the widget
        displayRect = rect();
    }
    
    // Draw the image
    std::lock_guard<std::mutex> lock(m_imageMutex);
    if (!m_displayImage.isNull()) {
        painter.drawImage(displayRect, m_displayImage);
    }
    
    // Draw any additional overlays
    // (e.g., debug information, FPS counter)
}

void VGADisplayWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    update(); // Trigger repaint
}

void VGADisplayWidget::updateDisplay()
{
    if (m_videoDevice && isVisible()) {
        // Update the image from the video device
        updateImageFromDevice();
        
        // Trigger a repaint
        update();
    }
}

void VGADisplayWidget::createDisplayImage()
{
    std::lock_guard<std::mutex> lock(m_imageMutex);
    
    int width = 640;   // Default width
    int height = 480;  // Default height
    
    if (m_videoDevice) {
        // Get dimensions from the video device
        width = m_videoDevice->GetWidth();
        height = m_videoDevice->GetHeight();
        
        // Adjust for text mode
        if (m_videoDevice->GetVideoMode() == VideoMode::TEXT) {
            width *= 8;   // 8 pixels per character
            height *= 16; // 16 pixels per character
        }
    }
    
    // Ensure minimum size
    width = std::max(320, width);
    height = std::max(240, height);
    
    // Create the display image
    m_displayImage = QImage(width, height, QImage::Format_ARGB32);
    m_displayImage.fill(Qt::black);
}

void VGADisplayWidget::updateImageFromDevice()
{
    if (!m_videoDevice || m_displayImage.isNull()) {
        return;
    }
    
    // Create or resize the image if needed
    int width = m_videoDevice->GetWidth();
    int height = m_videoDevice->GetHeight();
    
    // Adjust for text mode
    if (m_videoDevice->GetVideoMode() == VideoMode::TEXT) {
        width *= 8;   // 8 pixels per character
        height *= 16; // 16 pixels per character
    }
    
    if (m_displayImage.width() != width || m_displayImage.height() != height) {
        std::lock_guard<std::mutex> lock(m_imageMutex);
        m_displayImage = QImage(width, height, QImage::Format_ARGB32);
    }
    
    // Render the video device output directly to our image buffer
    if (m_videoDevice->GetVideoMode() == VideoMode::TEXT || 
        m_videoDevice->GetVideoMode() == VideoMode::GRAPHICS) {
        
        // Cast to SiS630VGA to access the RenderToTarget method
        SiS630VGA* sisVga = dynamic_cast<SiS630VGA*>(m_videoDevice);
        if (sisVga) {
            std::lock_guard<std::mutex> lock(m_imageMutex);
            sisVga->RenderToTarget(m_displayImage.bits(), 
                                 m_displayImage.width(), 
                                 m_displayImage.height());
        } else {
            // Fallback for other video devices
            const uint8_t* frameBuffer = m_videoDevice->GetFrameBuffer();
            if (frameBuffer) {
                // Simple copy of frame buffer based on bpp
                // This would need to be enhanced for different pixel formats
                int bpp = m_videoDevice->GetBitsPerPixel();
                
                // For demonstration, assuming the frame buffer is in a compatible format
                std::lock_guard<std::mutex> lock(m_imageMutex);
                
                if (bpp == 32) {
                    // Direct copy of 32-bit RGBA
                    std::memcpy(m_displayImage.bits(), frameBuffer, 
                              width * height * 4);
                } else {
                    // Would need additional format conversion
                    m_displayImage.fill(Qt::green); // Placeholder
                }
            }
        }
    }
}

} // namespace x86emu