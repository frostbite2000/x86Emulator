#pragma once

#include "x86emulator/sound_device.h"
#include <mutex>
#include <vector>

// Forward declarations for MAME types
namespace mame {
class ac97_device;
}

namespace x86emu {

/**
 * @brief AC97 audio codec implementation using MAME.
 * 
 * This class adapts MAME's AC97 implementation to the x86Emulator
 * SoundDevice interface.
 */
class AC97Device : public SoundDevice {
public:
    /**
     * Create an AC97 audio device.
     * @param name Unique device name
     */
    explicit AC97Device(const std::string& name);
    
    /**
     * Destructor.
     */
    ~AC97Device() override;

    // Device interface implementation
    bool Initialize() override;
    void Reset() override;
    void Update(uint32_t deltaTime) override;
    
    // SoundDevice interface implementation
    AudioFormat GetAudioFormat() const override;
    bool SetAudioFormat(const AudioFormat& format) override;
    int GetSamples(int16_t* buffer, int sampleCount) override;
    bool SupportsFormat(const AudioFormat& format) const override;
    int GetAvailableSamples() const override;
    void SetVolume(float level) override;
    float GetVolume() const override;
    void SetMuted(bool muted) override;
    bool IsMuted() const override;
    
private:
    // MAME AC97 device
    std::unique_ptr<mame::ac97_device> m_device;
    
    // Audio buffer
    std::vector<int16_t> m_audioBuffer;
    
    // Audio format
    AudioFormat m_format;
    
    // Volume control
    float m_volume;
    bool m_muted;
    
    // Access synchronization
    mutable std::mutex m_accessMutex;
    
    // Process audio
    void ProcessAudio(uint32_t deltaTime);
};

} // namespace x86emu