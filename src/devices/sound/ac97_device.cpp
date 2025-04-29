#include "ac97_device.h"
#include <algorithm>
#include <cassert>
#include <cstring>

// Include MAME AC97 headers
// Note: Paths may need adjustment based on MAME's directory structure in your project
#include "mame/src/devices/sound/ac97.h"

namespace x86emu {

AC97Device::AC97Device(const std::string& name)
    : SoundDevice(name, "AC97 Audio Codec")
    , m_volume(1.0f)
    , m_muted(false)
{
    // Set default audio format
    m_format.sampleRate = 48000; // AC97 uses 48kHz
    m_format.channels = 2;
    m_format.bitsPerSample = 16;
    m_format.signed_ = true;
    
    // Initialize audio buffer (100ms of audio)
    m_audioBuffer.resize(m_format.sampleRate / 10 * m_format.channels);
    std::fill(m_audioBuffer.begin(), m_audioBuffer.end(), 0);
}

AC97Device::~AC97Device()
{
    // MAME device will be cleaned up by its unique_ptr destructor
}

bool AC97Device::Initialize()
{
    if (IsInitialized()) {
        return true;
    }
    
    try {
        // Create the MAME AC97 device
        m_device = std::make_unique<mame::ac97_device>(0);
        if (!m_device) {
            return false;
        }
        
        // Initialize the device
        m_device->device_start();
        
        // Reset the device
        m_device->device_reset();
        
        SetInitialized(true);
        return true;
    }
    catch (const std::exception& ex) {
        // Log the exception
        return false;
    }
}

void AC97Device::Reset()
{
    if (IsInitialized() && m_device) {
        // Reset the MAME device
        m_device->device_reset();
        
        // Clear the audio buffer
        std::lock_guard<std::mutex> lock(m_accessMutex);
        std::fill(m_audioBuffer.begin(), m_audioBuffer.end(), 0);
    }
}

void AC97Device::Update(uint32_t deltaTime)
{
    if (IsInitialized()) {
        // Process audio for the elapsed time
        ProcessAudio(deltaTime);
    }
}

AudioFormat AC97Device::GetAudioFormat() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_format;
}

bool AC97Device::SetAudioFormat(const AudioFormat& format)
{
    if (!SupportsFormat(format)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Update the format
    m_format = format;
    
    // Resize the buffer
    m_audioBuffer.resize(m_format.sampleRate / 10 * m_format.channels);
    std::fill(m_audioBuffer.begin(), m_audioBuffer.end(), 0);
    
    return true;
}

int AC97Device::GetSamples(int16_t* buffer, int sampleCount)
{
    if (!IsInitialized() || !buffer || sampleCount <= 0) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // Calculate how many samples we can provide
    int available = std::min(static_cast<int>(m_audioBuffer.size()), sampleCount);
    
    if (available <= 0) {
        return 0;
    }
    
    // Apply volume if needed
    if (m_muted || m_volume <= 0.001f) {
        // Muted - fill with silence
        std::fill(buffer, buffer + available, 0);
    }
    else if (m_volume >= 0.999f) {
        // Full volume - direct copy
        std::memcpy(buffer, m_audioBuffer.data(), available * sizeof(int16_t));
    }
    else {
        // Apply volume
        for (int i = 0; i < available; ++i) {
            buffer[i] = static_cast<int16_t>(m_audioBuffer[i] * m_volume);
        }
    }
    
    // Remove used samples from buffer
    if (available < static_cast<int>(m_audioBuffer.size())) {
        std::copy(m_audioBuffer.begin() + available, m_audioBuffer.end(), m_audioBuffer.begin());
        m_audioBuffer.resize(m_audioBuffer.size() - available);
    }
    else {
        m_audioBuffer.clear();
    }
    
    return available;
}

bool AC97Device::SupportsFormat(const AudioFormat& format) const
{
    // AC97 supports 48kHz mono or stereo, 16-bit signed PCM
    
    // Check sample rate
    if (format.sampleRate != 48000) {
        return false;
    }
    
    // Check channels
    if (format.channels != 1 && format.channels != 2) {
        return false;
    }
    
    // Check bits per sample
    if (format.bitsPerSample != 16) {
        return false;
    }
    
    // Check signed
    if (!format.signed_) {
        return false;
    }
    
    return true;
}

int AC97Device::GetAvailableSamples() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return static_cast<int>(m_audioBuffer.size());
}

void AC97Device::SetVolume(float level)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_volume = std::max(0.0f, std::min(1.0f, level));
}

float AC97Device::GetVolume() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_volume;
}

void AC97Device::SetMuted(bool muted)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_muted = muted;
}

bool AC97Device::IsMuted() const
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_muted;
}

void AC97Device::ProcessAudio(uint32_t deltaTime)
{
    if (!IsInitialized() || m_muted) {
        return;
    }
    
    // Calculate the number of samples to generate
    int samplesToGenerate = (m_format.sampleRate * deltaTime) / 1000000;
    
    if (samplesToGenerate <= 0) {
        return;
    }
    
    // In a real implementation, we would get audio from the MAME device
    // This is simplified and would need to be adapted to the actual API
    
    std::lock_guard<std::mutex> lock(m_accessMutex);
    
    // For now, just generate silence
    std::vector<int16_t> tempBuffer(samplesToGenerate * m_format.channels);
    std::fill(tempBuffer.begin(), tempBuffer.end(), 0);
    
    // Add to main buffer
    m_audioBuffer.insert(m_audioBuffer.end(), tempBuffer.begin(), tempBuffer.end());
}

} // namespace x86emu