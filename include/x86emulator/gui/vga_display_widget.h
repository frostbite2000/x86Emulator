#pragma once

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <memory>
#include <mutex>
#include <vector>

namespace x86emu {

class VideoDevice;

/**
 * @brief Widget to display VGA output.
 * 
 * This widget renders the output from a VideoDevice to a Qt window.
 */
class VGADisplayWidget : public QWidget {
    Q_OBJECT
    
public:
    /**
     * Create a VGA display widget.
     * @param parent Parent widget
     */
    explicit VGADisplayWidget(QWidget *parent = nullptr);
    
    /**
     * Destructor.
     */
    ~VGADisplayWidget() override;
    
    /**
     * Set the video device to display.
     * @param device Video device to display
     */
    void SetVideoDevice(VideoDevice* device);
    
    /**
     * Set the display refresh rate.
     * @param fps Frames per second (default: 60)
     */
    void SetRefreshRate(int fps);
    
    /**
     * Get the current refresh rate.
     * @return Current refresh rate in FPS
     */
    int GetRefreshRate() const;
    
    /**
     * Enable or disable aspect ratio preservation.
     * @param preserve true to preserve aspect ratio
     */
    void PreserveAspectRatio(bool preserve);
    
    /**
     * Check if aspect ratio preservation is enabled.
     * @return true if aspect ratio is preserved
     */
    bool IsAspectRatioPreserved() const;
    
protected:
    /**
     * Paint event handler.
     * @param event Paint event
     */
    void paintEvent(QPaintEvent* event) override;
    
    /**
     * Resize event handler.
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    /**
     * Update the display.
     */
    void updateDisplay();
    
private:
    // Video device to display
    VideoDevice* m_videoDevice;
    
    // Display refresh timer
    QTimer m_refreshTimer;
    
    // Display settings
    int m_refreshRate;
    bool m_preserveAspectRatio;
    
    // Image buffer
    QImage m_displayImage;
    
    // Image buffer access synchronization
    std::mutex m_imageMutex;
    
    // Create or resize the display image
    void createDisplayImage();
    
    // Update the image from the video device
    void updateImageFromDevice();
};

} // namespace x86emu