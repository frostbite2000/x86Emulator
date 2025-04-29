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

#ifndef X86EMULATOR_DEVICE_H
#define X86EMULATOR_DEVICE_H

#include <string>

/**
 * @brief Base class for all emulated devices
 */
class Device {
public:
    /**
     * @brief Construct a new Device with a name
     * 
     * @param name Device name
     */
    explicit Device(const std::string& name);
    
    /**
     * @brief Destroy the Device
     */
    virtual ~Device();
    
    /**
     * @brief Get the device name
     * 
     * @return Device name
     */
    std::string getName() const;
    
    /**
     * @brief Initialize the device
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Update the device state
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    virtual void update(int cycles) = 0;
    
    /**
     * @brief Reset the device
     */
    virtual void reset() = 0;
    
    /**
     * @brief Check if this is a keyboard input device
     * 
     * @return true if this is a keyboard device
     * @return false if this is not a keyboard device
     */
    virtual bool isKeyboardDevice() const { return false; }
    
    /**
     * @brief Check if this is a mouse input device
     * 
     * @return true if this is a mouse device
     * @return false if this is not a mouse device
     */
    virtual bool isMouseDevice() const { return false; }
    
    /**
     * @brief Handle key press
     * 
     * @param key Key code
     * @param modifiers Key modifiers
     */
    virtual void keyPressed(int key, int modifiers) {}
    
    /**
     * @brief Handle key release
     * 
     * @param key Key code
     * @param modifiers Key modifiers
     */
    virtual void keyReleased(int key, int modifiers) {}
    
    /**
     * @brief Handle mouse button press
     * 
     * @param button Mouse button
     */
    virtual void mouseButtonPressed(int button) {}
    
    /**
     * @brief Handle mouse button release
     * 
     * @param button Mouse button
     */
    virtual void mouseButtonReleased(int button) {}
    
    /**
     * @brief Handle mouse movement
     * 
     * @param xRel Relative X movement
     * @param yRel Relative Y movement
     */
    virtual void mouseMove(int xRel, int yRel) {}
    
    /**
     * @brief Handle mouse wheel movement
     * 
     * @param delta Wheel delta
     */
    virtual void mouseWheelMove(int delta) {}
    
    /**
     * @brief Send Ctrl+Alt+Del keypress
     */
    virtual void sendCtrlAltDel() {}
    
    /**
     * @brief Check if the device is active
     * 
     * @return true if the device is active
     * @return false if the device is not active
     */
    virtual bool isActive() const { return false; }

private:
    std::string m_name;
};

#endif // X86EMULATOR_DEVICE_H