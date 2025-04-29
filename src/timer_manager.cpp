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

#include "timer_manager.h"
#include "logger.h"

TimerManager::TimerManager()
    : m_nextTimerId(1),
      m_cycleAccumulator(0),
      m_paused(false)
{
}

TimerManager::~TimerManager()
{
    // Clear all timers
    m_timers.clear();
}

bool TimerManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Clear timer list
        m_timers.clear();
        
        // Reset state
        m_nextTimerId = 1;
        m_cycleAccumulator = 0;
        m_paused = false;
        
        Logger::GetInstance()->info("Timer manager initialized");
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Timer manager initialization failed: %s", ex.what());
        return false;
    }
}

uint32_t TimerManager::createTimer(const std::string& name, TimerType type, uint32_t interval, TimerCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if timer with this name already exists
    for (const auto& timer : m_timers) {
        if (timer.name == name) {
            Logger::GetInstance()->warn("Timer with name '%s' already exists", name.c_str());
            return 0;
        }
    }
    
    // Check if interval is valid
    if (interval == 0) {
        Logger::GetInstance()->error("Timer interval cannot be zero");
        return 0;
    }
    
    // Create new timer
    Timer timer;
    timer.id = m_nextTimerId++;
    timer.name = name;
    timer.active = false;
    timer.type = type;
    timer.interval = interval;
    timer.countdown = interval;
    timer.callback = callback;
    
    // Add to timer list
    m_timers.push_back(timer);
    
    Logger::GetInstance()->info("Created timer '%s' (ID %u) with %s interval %u µs",
                              name.c_str(), timer.id,
                              (type == TimerType::ONESHOT ? "one-shot" : "periodic"),
                              interval);
    
    return timer.id;
}

bool TimerManager::startTimer(uint32_t id, uint32_t initialDelay)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find timer
    Timer* timer = getTimerMutable(id);
    if (!timer) {
        Logger::GetInstance()->warn("Timer ID %u not found", id);
        return false;
    }
    
    // Set initial countdown
    timer->countdown = initialDelay > 0 ? initialDelay : timer->interval;
    
    // Activate timer
    timer->active = true;
    
    Logger::GetInstance()->debug("Started timer '%s' (ID %u)", timer->name.c_str(), timer->id);
    return true;
}

bool TimerManager::stopTimer(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find timer
    Timer* timer = getTimerMutable(id);
    if (!timer) {
        Logger::GetInstance()->warn("Timer ID %u not found", id);
        return false;
    }
    
    // Deactivate timer
    timer->active = false;
    
    Logger::GetInstance()->debug("Stopped timer '%s' (ID %u)", timer->name.c_str(), timer->id);
    return true;
}

bool TimerManager::destroyTimer(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find timer
    for (auto it = m_timers.begin(); it != m_timers.end(); ++it) {
        if (it->id == id) {
            // Remove timer
            std::string name = it->name;
            m_timers.erase(it);
            
            Logger::GetInstance()->info("Destroyed timer '%s' (ID %u)", name.c_str(), id);
            return true;
        }
    }
    
    Logger::GetInstance()->warn("Timer ID %u not found", id);
    return false;
}

uint32_t TimerManager::findTimer(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find timer by name
    for (const auto& timer : m_timers) {
        if (timer.name == name) {
            return timer.id;
        }
    }
    
    return 0;
}

const TimerManager::Timer* TimerManager::getTimer(uint32_t id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find timer by ID
    for (const auto& timer : m_timers) {
        if (timer.id == id) {
            return &timer;
        }
    }
    
    return nullptr;
}

void TimerManager::update(int cycles)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Skip update if paused
    if (m_paused) {
        return;
    }
    
    // Convert cycles to microseconds
    uint32_t microseconds = static_cast<uint32_t>(cycles / CYCLES_PER_MICROSECOND);
    
    // Add fractional cycles to accumulator
    m_cycleAccumulator += cycles % static_cast<uint64_t>(CYCLES_PER_MICROSECOND);
    
    // Convert accumulated cycles to microseconds
    if (m_cycleAccumulator >= CYCLES_PER_MICROSECOND) {
        microseconds += static_cast<uint32_t>(m_cycleAccumulator / CYCLES_PER_MICROSECOND);
        m_cycleAccumulator %= static_cast<uint64_t>(CYCLES_PER_MICROSECOND);
    }
    
    // Update all active timers
    for (auto& timer : m_timers) {
        if (timer.active) {
            // Decrement countdown
            if (timer.countdown <= microseconds) {
                // Timer expired
                fireTimer(timer);
                
                // Reset countdown for periodic timers
                if (timer.type == TimerType::PERIODIC) {
                    // Handle case where elapsed time > interval
                    uint32_t elapsed = microseconds - timer.countdown;
                    uint32_t periods = 1 + (elapsed / timer.interval);
                    
                    // Fire timer for each full period
                    for (uint32_t i = 1; i < periods; ++i) {
                        fireTimer(timer);
                    }
                    
                    // Set new countdown
                    timer.countdown = timer.interval - (elapsed % timer.interval);
                } else {
                    // Deactivate one-shot timer
                    timer.active = false;
                }
            } else {
                // Decrement countdown
                timer.countdown -= microseconds;
            }
        }
    }
}

void TimerManager::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset all timers
    for (auto& timer : m_timers) {
        timer.active = false;
        timer.countdown = timer.interval;
    }
    
    // Reset accumulator
    m_cycleAccumulator = 0;
    
    // Clear paused state
    m_paused = false;
    
    Logger::GetInstance()->info("All timers reset");
}

void TimerManager::pause()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_paused) {
        m_paused = true;
        Logger::GetInstance()->info("Timer manager paused");
    }
}

void TimerManager::resume()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_paused) {
        m_paused = false;
        Logger::GetInstance()->info("Timer manager resumed");
    }
}

TimerManager::Timer* TimerManager::getTimerMutable(uint32_t id)
{
    // Find timer by ID
    for (auto& timer : m_timers) {
        if (timer.id == id) {
            return &timer;
        }
    }
    
    return nullptr;
}

void TimerManager::fireTimer(Timer& timer)
{
    // Call timer callback
    if (timer.callback) {
        try {
            timer.callback(timer.id, timer.interval);
        } catch (const std::exception& ex) {
            Logger::GetInstance()->error("Exception in timer callback for '%s' (ID %u): %s",
                                      timer.name.c_str(), timer.id, ex.what());
        }
    }
    
    Logger::GetInstance()->debug("Timer '%s' (ID %u) fired", timer.name.c_str(), timer.id);
}