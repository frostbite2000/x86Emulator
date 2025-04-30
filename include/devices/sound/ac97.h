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

#ifndef X86EMULATOR_AC97_H
#define X86EMULATOR_AC97_H

#include <cstdint>
#include <vector>
#include <functional>
#include <mutex>

namespace x86emu {

/**
 * @brief AC97 Audio Controller
 * 
 * This class implements the AC97 audio controller functionality
 * as defined by the AC97 specification.
 */
class AC97AudioController {
public:
    /**
     * @brief Construct a new AC97 Audio Controller
     */
    AC97AudioController();
    
    /**
     * @brief Destroy the AC97 Audio Controller
     */
    ~AC97AudioController();
    
    /**
     * @brief Initialize the AC97 controller
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Reset the AC97 controller
     */
    void reset();
    
    /**
     * @brief Read from an AC97 register
     * 
     * @param reg Register offset
     * @return Register value
     */
    uint8_t readRegister(uint16_t reg);
    
    /**
     * @brief Write to an AC97 register
     * 
     * @param reg Register offset
     * @param value Value to write
     */
    void writeRegister(uint16_t reg, uint8_t value);
    
    /**
     * @brief Set audio data callback
     * 
     * @param callback Function to call with audio data
     */
    void setAudioCallback(std::function<void(const int16_t*, size_t)> callback);

private:
    // AC97 registers
    struct {
        uint16_t reset;             // 0x00: Reset
        uint16_t masterVolume;      // 0x02: Master Volume
        uint16_t headphoneVolume;   // 0x04: Headphone Volume
        uint16_t masterMono;        // 0x06: Master Mono
        uint16_t masterTone;        // 0x08: Master Tone (Bass, Treble)
        uint16_t pcBeepVolume;      // 0x0A: PC Beep Volume
        uint16_t phoneVolume;       // 0x0C: Phone Volume
        uint16_t micVolume;         // 0x0E: Mic Volume
        uint16_t lineInVolume;      // 0x10: Line In Volume
        uint16_t cdVolume;          // 0x12: CD Volume
        uint16_t videoVolume;       // 0x14: Video Volume
        uint16_t auxVolume;         // 0x16: Aux Volume
        uint16_t pcmOutVolume;      // 0x18: PCM Out Volume
        uint16_t recordSelect;      // 0x1A: Record Select
        uint16_t recordGain;        // 0x1C: Record Gain
        uint16_t recordGainMic;     // 0x1E: Record Gain Mic
        uint16_t generalPurpose;    // 0x20: General Purpose
        uint16_t control3D;         // 0x22: 3D Control
        uint16_t audioIntStatus;    // 0x24: Audio Interrupt Status
        uint16_t powerdownCtrlSts;  // 0x26: Powerdown Control/Status
        uint16_t extAudioID;        // 0x28: Extended Audio ID
        uint16_t extAudioCtrlSts;   // 0x2A: Extended Audio Control/Status
        uint16_t pcmFrontDacRate;   // 0x2C: PCM Front DAC Rate
        uint16_t pcmSurroundDacRate;// 0x2E: PCM Surround DAC Rate
        uint16_t pcmLFEDacRate;     // 0x30: PCM LFE DAC Rate
        uint16_t pcmInputDacRate;   // 0x32: PCM Input DAC Rate
        uint16_t centerLFEVolume;   // 0x36: Center/LFE Volume
        uint16_t surroundVolume;    // 0x38: Surround Volume
        uint16_t spdifControl;      // 0x3A: S/PDIF Control
        uint16_t vendorID1;         // 0x7C: Vendor ID1
        uint16_t vendorID2;         // 0x7E: Vendor ID2
    } m_regs;
    
    // Audio buffer
    std::vector<int16_t> m_audioBuffer;
    std::mutex m_audioMutex;
    
    // Audio callback
    std::function<void(const int16_t*, size_t)> m_audioCallback;
    
    // Buffer descriptor list (BDL) for DMA
    struct {
        uint32_t bufferAddress;
        uint32_t bufferSize;
        uint32_t status;
        bool enabled;
    } m_bdl[32];
    
    // Helper methods
    uint16_t readRegister16(uint16_t reg);
    void writeRegister16(uint16_t reg, uint16_t value);
    void generateAudio();
};

} // namespace x86emu

#endif // X86EMULATOR_AC97_H