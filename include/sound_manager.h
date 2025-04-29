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

#ifndef X86EMULATOR_SOUND_MANAGER_H
#define X86EMULATOR_SOUND_MANAGER_H

#include "device.h"
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include <QObject>
#include <QAudioOutput>
#include <QByteArray>
#include <QBuffer>

class SoundDevice;

/**
 * @brief Sound manager for the emulator
 * 
 * Manages sound devices and audio output
 */
class SoundManager : public QObject, public Device {
    Q_OBJECT
public:
    /**
     * @brief Construct a new Sound Manager
     * 
     * @param cardType Sound card type (e.g., "sb16", "adlib", "none")
     */
    explicit SoundManager(const std::string& cardType);
    
    /**
     * @brief Destroy the Sound Manager
     */
    ~SoundManager() override;
    
    /**
     * @brief Initialize the sound system
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Update the sound system
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles) override;
    
    /**
     * @brief Reset the sound system
     */
    void reset() override;
    
    /**
     * @brief Set the mute state
     * 
     * @param mute true to mute, false to unmute
     */
    void setMute(bool mute);
    
    /**
     * @brief Set the volume level
     * 
     * @param volume Volume level (0-100)
     */
    void setVolume(int volume);
    
    /**
     * @brief Get the mute state
     * 
     * @return true if muted
     * @return false if not muted
     */
    bool isMuted() const;
    
    /**
     * @brief Get the volume level
     * 
     * @return Volume level (0-100)
     */
    int getVolume() const;
    
    /**
     * @brief Add audio samples to the output buffer
     * 
     * @param samples Samples to add (16-bit signed PCM)
     * @param count Number of samples
     */
    void addSamples(const int16_t* samples, size_t count);
    
    /**
     * @brief Read from a sound register
     * 
     * @param port Register port
     * @return Register value
     */
    uint8_t readRegister(uint16_t port) const;
    
    /**
     * @brief Write to a sound register
     * 
     * @param port Register port
     * @param value Value to write
     */
    void writeRegister(uint16_t port, uint8_t value);
    
    /**
     * @brief Get the sound card type
     * 
     * @return Sound card type
     */
    std::string getCardType() const;

private slots:
    void handleAudioStateChanged(QAudio::State state);
    
private:
    // Sound card type
    std::string m_cardType;
    
    // Sound device
    std::unique_ptr<SoundDevice> m_soundDevice;
    
    // Audio output
    QAudioOutput* m_audioOutput;
    QByteArray m_audioBuffer;
    QBuffer m_buffer;
    
    // Audio parameters
    int m_sampleRate;
    int m_channels;
    int m_volume;
    bool m_muted;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;
    
    // Helper methods
    void setupAudio();
    void cleanupAudio();
    void flushAudioBuffer();
};

#endif // X86EMULATOR_SOUND_MANAGER_H