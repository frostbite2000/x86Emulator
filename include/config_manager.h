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

#ifndef X86EMULATOR_CONFIG_MANAGER_H
#define X86EMULATOR_CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

/**
 * @brief Manager for emulator configuration
 * 
 * Handles loading, saving, and accessing configuration settings
 * with support for different data types and default values.
 */
class ConfigManager {
public:
    /**
     * @brief Construct a new ConfigManager
     * 
     * @param configPath Path to the configuration file (optional)
     */
    ConfigManager(const std::string& configPath = "x86emulator.cfg");
    
    /**
     * @brief Destroy the ConfigManager
     */
    ~ConfigManager();
    
    /**
     * @brief Load configuration from file
     * 
     * @return true if configuration was loaded successfully
     * @return false if loading failed
     */
    bool loadConfiguration();
    
    /**
     * @brief Save configuration to file
     * 
     * @return true if configuration was saved successfully
     * @return false if saving failed
     */
    bool saveConfiguration();
    
    /**
     * @brief Get a string value from the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return The string value
     */
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    
    /**
     * @brief Get an integer value from the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return The integer value
     */
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    
    /**
     * @brief Get a double value from the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return The double value
     */
    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const;
    
    /**
     * @brief Get a boolean value from the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return The boolean value
     */
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const;
    
    /**
     * @brief Get a string list from the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return The string list
     */
    std::vector<std::string> getStringList(const std::string& section, const std::string& key, const std::vector<std::string>& defaultValue = std::vector<std::string>()) const;
    
    /**
     * @brief Set a string value in the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param value String value to set
     */
    void setString(const std::string& section, const std::string& key, const std::string& value);
    
    /**
     * @brief Set an integer value in the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param value Integer value to set
     */
    void setInt(const std::string& section, const std::string& key, int value);
    
    /**
     * @brief Set a double value in the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param value Double value to set
     */
    void setDouble(const std::string& section, const std::string& key, double value);
    
    /**
     * @brief Set a boolean value in the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param value Boolean value to set
     */
    void setBool(const std::string& section, const std::string& key, bool value);
    
    /**
     * @brief Set a string list in the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @param value String list to set
     */
    void setStringList(const std::string& section, const std::string& key, const std::vector<std::string>& value);
    
    /**
     * @brief Check if a key exists in the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     * @return true if the key exists
     * @return false if the key does not exist
     */
    bool hasKey(const std::string& section, const std::string& key) const;
    
    /**
     * @brief Remove a key from the configuration
     * 
     * @param section Configuration section
     * @param key Configuration key
     */
    void removeKey(const std::string& section, const std::string& key);
    
    /**
     * @brief Get the singleton instance of the ConfigManager
     * 
     * @return Pointer to the ConfigManager instance
     */
    static ConfigManager* GetInstance();

private:
    // Configuration storage: section -> key -> value
    using ConfigSection = std::map<std::string, std::string>;
    using ConfigData = std::map<std::string, ConfigSection>;
    
    ConfigData m_configData;
    std::string m_configPath;
    mutable std::recursive_mutex m_mutex;
    
    // Singleton instance
    static std::unique_ptr<ConfigManager> s_instance;
    static std::once_flag s_onceFlag;
    
    // Helper methods
    void parseLine(const std::string& line, std::string& currentSection);
    std::string serializeValue(const std::string& value) const;
    std::string deserializeValue(const std::string& value) const;
    std::string escapeString(const std::string& str) const;
    std::string unescapeString(const std::string& str) const;
};

#endif // X86EMULATOR_CONFIG_MANAGER_H