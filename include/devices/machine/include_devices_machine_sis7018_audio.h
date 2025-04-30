/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2025 frostbite2000
 * Portions Copyright (C) 2011-2025 MAME development team (original SiS 7018 implementation)
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

#ifndef X86EMULATOR_SIS7018_AUDIO_H
#define X86EMULATOR_SIS7018_AUDIO_H

#include "devices/pci/pci_device.h"
#include "memory_manager.h"
#include "io_manager.h"
#include "logger.h"
#include <array>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>

namespace x86emu {

// Forward declaration
class AC97AudioController;
class JoyportDevice;

/**
 * @brief SiS 7018 Audio Controller (AC97 compliant)
 * 
 * This device emulates the SiS 7018 Audio Controller, which is based on
 * Trident 4DWave technology. It provides AC97 audio functionality
 * along with a gameport interface.
 */
class SiS7018AudioDevice : public PCIDevice {
public:
    /**
     * @brief Construct a new SiS 7018 Audio Controller
     */
    SiS7018AudioDevice();
    
    /**
     * @brief Destroy the SiS 7018 Audio Controller
     */
    ~SiS7018AudioDevice() override;
    
    /**
     * @brief Initialize the audio controller
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Reset the audio controller
     */
    void reset() override;
    
    /**
     * @brief Map memory regions and I/O ports
     * 
     * @param memory_manager Memory manager instance
     * @param io_manager I/O manager instance
     */
    void mapSpecialRegions(MemoryManager* memory_manager, IOManager* io_manager) override;
    
    /**
     * @brief Register an audio buffer callback
     * 
     * @param callback Function to call with audio data
     */
    void registerAudioCallback(std::function<void(const int16_t*, size_t)> callback);
    
    /**
     * @brief Register a joystick state update callback
     * 
     * @param callback Function to call when joystick state is read
     */
    void registerJoyCallback(std::function<void(uint8_t*, uint8_t*)> callback);

private:
    // AC97 Audio Controller
    std::unique_ptr<AC97AudioController> m_ac97;
    
    // Joystick/Gameport
    std::unique_ptr<JoyportDevice> m_joyport;
    
    // Callbacks
    std::function<void(const int16_t*, size_t)> m_audioCallback;
    std::function<void(uint8_t*, uint8_t*)> m_joyCallback;
    
    // Legacy I/O base addresses
    uint8_t m_legacyIOBase;
    
    // Memory region
    std::vector<uint8_t> m_mmioRegs;
    uint32_t m_mmioBase;
    std::mutex m_mmioMutex;
    
    // Audio buffer
    std::vector<int16_t> m_audioBuffer;
    std::mutex m_audioMutex;
    
    // PCI Configuration registers
    uint8_t readCapPtr();
    uint32_t readPmcId();
    
    // Legacy I/O handlers
    void handleLegacyIORead(uint16_t port, uint8_t* value);
    void handleLegacyIOWrite(uint16_t port, uint8_t value);
    
    // MMIO handlers
    void handleMemoryRead(uint32_t address, uint32_t* value);
    void handleMemoryWrite(uint32_t address, uint32_t value);
    
    // Audio functions
    void generateAudioSamples();
    void submitAudioBuffer();
    
    // Debug helpers
    void debugUnhandledRead(uint16_t port);
    void debugUnhandledWrite(uint16_t port, uint8_t value);
};

} // namespace x86emu

#endif // X86EMULATOR_SIS7018_AUDIO_H