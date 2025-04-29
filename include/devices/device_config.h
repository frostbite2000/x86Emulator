/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2024 frostbite2000
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

#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <string>
#include <unordered_map>
#include <vector>
#include <any>
#include <stdexcept>
#include <typeinfo>
#include <memory>

/**
 * @brief Configuration class for device parameters.
 * 
 * This class provides a flexible way to store and retrieve configuration
 * parameters for device initialization.
 */
class DeviceConfig {
public:
    /**
     * @brief Default constructor.
     */
    DeviceConfig() = default;
    
    /**
     * @brief Constructor with initial parameters.
     * 
     * @param params Initial parameters as key-value pairs.
     */
    DeviceConfig(const std::unordered_map<std::string, std::any>& params) 
        : m_params(params) {}
    
    /**
     * @brief Set a parameter value.
     * 
     * @tparam T Type of the parameter value.
     * @param key Parameter name.
     * @param value Parameter value.
     */
    template<typename T>
    void set(const std::string& key, const T& value) {
        m_params[key] = value;
    }
    
    /**
     * @brief Get a parameter value.
     * 
     * @tparam T Expected type of the parameter value.
     * @param key Parameter name.
     * @param defaultValue Default value to return if parameter is not found.
     * @return T The parameter value or defaultValue if not found.
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const {
        auto it = m_params.find(key);
        if (it == m_params.end()) {
            return defaultValue;
        }
        
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            return defaultValue;
        }
    }
    
    /**
     * @brief Check if a parameter exists.
     * 
     * @param key Parameter name.
     * @return bool True if parameter exists, false otherwise.
     */
    bool hasParam(const std::string& key) const {
        return m_params.find(key) != m_params.end();
    }
    
    /**
     * @brief Get the type name of a parameter.
     * 
     * @param key Parameter name.
     * @return std::string Type name of the parameter or empty string if not found.
     */
    std::string getParamType(const std::string& key) const {
        auto it = m_params.find(key);
        if (it == m_params.end()) {
            return "";
        }
        
        return it->second.type().name();
    }
    
    /**
     * @brief Get a list of all parameter names.
     * 
     * @return std::vector<std::string> List of parameter names.
     */
    std::vector<std::string> getParamNames() const {
        std::vector<std::string> names;
        names.reserve(m_params.size());
        
        for (const auto& param : m_params) {
            names.push_back(param.first);
        }
        
        return names;
    }
    
    /**
     * @brief Get the number of parameters.
     * 
     * @return size_t Number of parameters.
     */
    size_t size() const {
        return m_params.size();
    }
    
    /**
     * @brief Clear all parameters.
     */
    void clear() {
        m_params.clear();
    }
    
    /**
     * @brief Remove a parameter.
     * 
     * @param key Parameter name.
     * @return bool True if parameter was removed, false if not found.
     */
    bool remove(const std::string& key) {
        return m_params.erase(key) > 0;
    }
    
    /**
     * @brief Get a reference to the underlying parameter map.
     * 
     * @return const std::unordered_map<std::string, std::any>& Reference to the parameter map.
     */
    const std::unordered_map<std::string, std::any>& getParamMap() const {
        return m_params;
    }

private:
    /**
     * @brief Map of parameter names to values.
     */
    std::unordered_map<std::string, std::any> m_params;
};

#endif // DEVICE_CONFIG_H