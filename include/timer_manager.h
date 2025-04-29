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

#ifndef X86EMULATOR_TIMER_MANAGER_H
#define X86EMULATOR_TIMER_MANAGER_H

#include <cstdint>
#include <functional>
#include <vector>
#include <string>
#include <mutex>

/**
 * @brief Manager for system timers
 * 
 * Handles timer creation, management, and updates
 */
class TimerManager {
public:
    /**
     * @brief Timer callback type
     */
    using TimerCallback = std::function<void(uint32_t, uint32_t)>;
    
    /**
     * @brief Timer types
     */
    enum class TimerType {
        ONESHOT,    ///< Fire once then stop
        PERIODIC    ///< Fire periodically
    };
    
    /**
     * @brief Timer structure
     */
    struct Timer {
        uint32_t id;             ///< Timer ID
        std::string name;        ///< Timer name
        bool active;             ///< Is timer active
        TimerType type;          ///< Timer type
        uint32_t interval;       ///< Timer interval in microseconds
        uint32_t countdown;      ///< Current countdown in microseconds
        TimerCallback callback;  ///< Callback function
    };
    
    /**
     * @brief Construct a new Timer Manager
     */
    TimerManager();
    
    /**
     * @brief Destroy the Timer Manager
     */
    ~TimerManager();
    
    /**
     * @brief Initialize the timer system
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();
    
    /**
     * @brief Create a new timer
     * 
     * @param name Timer name
     * @param type Timer type
     * @param interval Timer interval in microseconds
     * @param callback Callback function to call when timer expires
     * @return Timer ID, or 0 if creation failed
     */
    uint32_t createTimer(const std::string& name, TimerType type, uint32_t interval, TimerCallback callback);
    
    /**
     * @brief Start a timer
     * 
     * @param id Timer ID
     * @param initialDelay Initial delay in microseconds (or 0 for immediate start)
     * @return true if timer was started
     * @return false if timer could not be started
     */
    bool startTimer(uint32_t id, uint32_t initialDelay = 0);
    
    /**
     * @brief Stop a timer
     * 
     * @param id Timer ID
     * @return true if timer was stopped
     * @return false if timer could not be stopped
     */
    bool stopTimer(uint32_t id);
    
    /**
     * @brief Destroy a timer
     * 
     * @param id Timer ID
     * @return true if timer was destroyed
     * @return false if timer could not be destroyed
     */
    bool destroyTimer(uint32_t id);
    
    /**
     * @brief Find a timer by name
     * 
     * @param name Timer name
     * @return Timer ID, or 0 if not found
     */
    uint32_t findTimer(const std::string& name) const;
    
    /**
     * @brief Get a timer by ID
     * 
     * @param id Timer ID
     * @return Pointer to timer, or nullptr if not found
     */
    const Timer* getTimer(uint32_t id) const;
    
    /**
     * @brief Update all timers
     * 
     * @param cycles Number of CPU cycles elapsed
     */
    void update(int cycles);
    
    /**
     * @brief Reset all timers
     */
    void reset();
    
    /**
     * @brief Pause all timers
     */
    void pause();
    
    /**
     * @brief Resume all timers
     */
    void resume();

private:
    std::vector<Timer> m_timers;
    uint32_t m_nextTimerId;
    uint64_t m_cycleAccumulator;
    bool m_paused;
    mutable std::mutex m_mutex;
    
    // System constants
    static constexpr double CYCLES_PER_MICROSECOND = 4.77; // 4.77 MHz
    
    // Helper methods
    Timer* getTimerMutable(uint32_t id);
    void fireTimer(Timer& timer);
};

#endif // X86EMULATOR_TIMER_MANAGER_H