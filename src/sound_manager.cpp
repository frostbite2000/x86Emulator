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

#include "sound_manager.h"
#include "sound_device.h"
#include "logger.h"

#include <QAudioFormat>
#include <algorithm>

SoundManager::SoundManager(const std::string& cardType)
    : Device("SoundManager_" + cardType),
      m_cardType(cardType),
      m_audioOutput(nullptr),
      m_sampleRate(44100),
      m_channels(2),
      m_volume(100),
      m_muted(false)
{
}

SoundManager::~SoundManager()
{
    // Cleanup audio
    cleanupAudio();
}

bool SoundManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Create sound device based on card type
        if (m_cardType == "sb16") {
            // m_soundDevice = std::make_unique<SoundBlaster16>();
        } else if (m_cardType == "adlib") {
            // m_soundDevice = std::make_unique<AdLib>();
        } else if (m_cardType == "none") {
            // No sound device
        } else {
            Logger::GetInstance()->warn("Unknown sound card type: %s, defaulting to none", m_cardType.c_str());
            m_cardType = "none";
        }
        
        // Initialize sound device
        if (m_soundDevice && !m_soundDevice->initialize()) {
            Logger::GetInstance()->error("Sound device initialization failed");
            return false;
        }
        
        // Setup audio output
        setupAudio();
        
        Logger::GetInstance()->info("Sound manager initialized: %s", m_cardType.c_str());
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Sound manager initialization failed: %s", ex.what());
        return false;
    }
}

void SoundManager::update(int cycles)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update sound device
    if (m_soundDevice) {
        m_soundDevice->update(cycles);
    }
    
    // Flush audio buffer if needed
    flushAudioBuffer();
}

void SoundManager::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset sound device
    if (m_soundDevice) {
        m_soundDevice->reset();
    }
    
    // Clear audio buffer
    m_audioBuffer.clear();
    m_buffer.close();
    m_buffer.setBuffer(&m_audioBuffer);
    m_buffer.open(QIODevice::ReadWrite);
    
    Logger::GetInstance()->info("Sound manager reset");
}

void SoundManager::setMute(bool mute)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_muted = mute;
    
    // Update audio output
    if (m_audioOutput) {
        m_audioOutput->setMuted(mute);
    }
    
    Logger::GetInstance()->info("Sound %s", mute ? "muted" : "unmuted");
}

void SoundManager::setVolume(int volume)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clamp volume to 0-100
    m_volume = std::max(0, std::min(100, volume));
    
    // Update audio output
    if (m_audioOutput) {
        m_audioOutput->setVolume(m_volume / 100.0f);
    }
    
    Logger::GetInstance()->info("Sound volume set to %d%%", m_volume);
}

bool SoundManager::isMuted() const
{
    return m_muted;
}

int SoundManager::getVolume() const
{
    return m_volume;
}

void SoundManager::addSamples(const int16_t* samples, size_t count)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Skip if audio is not initialized or muted
    if (!m_audioOutput || m_muted) {
        return;
    }
    
    // Add samples to buffer
    qint64 bytesToWrite = count * sizeof(int16_t);
    qint64 pos = m_buffer.pos();
    
    // Ensure buffer has enough space
    if (pos + bytesToWrite > m_audioBuffer.size()) {
        m_audioBuffer.resize(pos + bytesToWrite);
    }
    
    // Write samples to buffer
    m_buffer.write(reinterpret_cast<const char*>(samples), bytesToWrite);
    
    // Flush buffer if it's getting too large
    if (m_buffer.pos() > 16384) {
        flushAudioBuffer();
    }
}

uint8_t SoundManager::readRegister(uint16_t port) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Forward to sound device
    if (m_soundDevice) {
        return m_soundDevice->readRegister(port);
    }
    
    return 0xFF;
}

void SoundManager::writeRegister(uint16_t port, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Forward to sound device
    if (m_soundDevice) {
        m_soundDevice->writeRegister(port, value);
    }
}

std::string SoundManager::getCardType() const
{
    return m_cardType;
}

void SoundManager::handleAudioStateChanged(QAudio::State state)
{
    switch (state) {
        case QAudio::ActiveState:
            Logger::GetInstance()->debug("Audio stream active");
            break;
        case QAudio::SuspendedState:
            Logger::GetInstance()->debug("Audio stream suspended");
            break;
        case QAudio::StoppedState:
            Logger::GetInstance()->debug("Audio stream stopped");
            break;
        case QAudio::IdleState:
            Logger::GetInstance()->debug("Audio stream idle");
            break;
    }
}

void SoundManager::setupAudio()
{
    // Clean up any existing audio setup
    cleanupAudio();
    
    // Create audio format
    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    
    // Check if format is supported
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        Logger::GetInstance()->warn("Audio format not supported, using nearest");
        format = info.nearestFormat(format);
    }
    
    // Create audio output
    m_audioOutput = new QAudioOutput(format, this);
    m_audioOutput->setVolume(m_volume / 100.0f);
    m_audioOutput->setMuted(m_muted);
    
    // Connect signals
    connect(m_audioOutput, &QAudioOutput::stateChanged, this, &SoundManager::handleAudioStateChanged);
    
    // Prepare buffer
    m_audioBuffer.clear();
    m_buffer.setBuffer(&m_audioBuffer);
    m_buffer.open(QIODevice::ReadWrite);
    
    // Start audio
    m_audioOutput->start(&m_buffer);
    
    Logger::GetInstance()->info("Audio output initialized: %d Hz, %d channels",
                              m_sampleRate, m_channels);
}

void SoundManager::cleanupAudio()
{
    // Stop and delete audio output
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
    
    // Close buffer
    m_buffer.close();
    m_audioBuffer.clear();
}

void SoundManager::flushAudioBuffer()
{
    // Skip if no audio output
    if (!m_audioOutput) {
        return;
    }
    
    // Check buffer state
    if (m_buffer.pos() == 0) {
        return;
    }
    
    // Rewind buffer for reading
    m_buffer.seek(0);
    
    // Get free bytes in audio output buffer
    qint64 freeBytes = m_audioOutput->bytesFree();
    if (freeBytes == 0) {
        return;
    }
    
    // Calculate bytes to write
    qint64 bytesToWrite = std::min(freeBytes, m_buffer.size());
    
    // Write to audio output
    qint64 bytesWritten = m_audioOutput->write(m_buffer.read(bytesToWrite));
    
    // Reset buffer if all data was written
    if (m_buffer.atEnd()) {
        m_audioBuffer.clear();
        m_buffer.close();
        m_buffer.setBuffer(&m_audioBuffer);
        m_buffer.open(QIODevice::ReadWrite);
    } else {
        // Compact buffer
        m_audioBuffer = m_audioBuffer.mid(bytesWritten);
        m_buffer.close();
        m_buffer.setBuffer(&m_audioBuffer);
        m_buffer.open(QIODevice::ReadWrite);
        m_buffer.seek(m_audioBuffer.size());
    }
}