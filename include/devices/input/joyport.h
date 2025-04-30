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

#ifndef X86EMULATOR_JOYPORT_H
#define X86EMULATOR_JOYPORT_H

#include <cstdint>
#include <functional>
#include <chrono>
#include <array>

namespace x86emu {

/**
 * @brief PC Joystick/Gameport device
 * 
 * This class emulates the standard PC gameport interface.
 */
class JoyportDevice {
public:
    /**
     * @brief Construct a new Joyport Device
     */
    JoyportDevice();
    
    /**
     * @brief Destroy the Joyport Device
     */
    ~JoyportDevice();
    
    /**
     * @brief Initialize the gameport
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Reset the gameport
     */
    void reset();
    
    /**
     * @brief Read from a gameport register
     * 
     * @param offset Register offset (0-7)
     * @return Register value
     */
    uint8_t read(uint8_t offset);
    
    /**
     * @brief Write to a gameport register
     * 
     * @param offset Register offset (0-7)
     * @param value Value to write
     */
    void write(uint8_t offset, uint8_t value);
    
    /**
     * @brief Set the state of the joystick buttons
     * 
     * @param buttons Button state for all four joysticks (bit 0-3)
     */
    void setButtonState(uint8_t buttons);
    
    /**
     * @brief Set the axis values for both joysticks
     * 
     * @param x1 Joystick 1 X-axis position (0-255)
     * @param y1 Joystick 1 Y-axis position (0-255)
     * @param x2 Joystick 2 X-axis position (0-255)
     * @param y2 Joystick 2 Y-axis position (0-255)
     */
    void setAxisValues(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    
    /**
     * @brief Set the state callback function
     * 
     * @param callback Function to call when joystick state is read
     */
    void setStateCallback(std::function<void(uint8_t*, uint8_t*)> callback);

private:
    // Joystick state
    uint8_t m_buttons;
    std::array<uint8_t, 4> m_axisValues; // X1, Y1, X2, Y2
    
    // Timing variables
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
    bool m_triggerActive;
    
    // Callback
    std::function<void(uint8_t*, uint8_t*)> m_stateCallback;
};

} // namespace x86emu

#endif // X86EMULATOR_JOYPORT_H