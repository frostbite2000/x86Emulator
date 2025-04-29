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

#include "config_manager.h"
#include "logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <regex>
#include <stdexcept>
#include <filesystem>

// Initialize static members
std::unique_ptr<ConfigManager> ConfigManager::s_instance = nullptr;
std::once_flag ConfigManager::s_onceFlag;

ConfigManager::ConfigManager(const std::string& configPath)
    : m_configPath(configPath)
{
    // Try to load the configuration file
    loadConfiguration();
}

ConfigManager::~ConfigManager()
{
    // Save configuration on destruction
    saveConfiguration();
}

bool ConfigManager::loadConfiguration()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try {
        // Check if the file exists
        if (!std::filesystem::exists(m_configPath)) {
            return false;
        }
        
        // Open the config file
        std::ifstream file(m_configPath);
        if (!file.is_open()) {
            return false;
        }
        
        // Clear existing data
        m_configData.clear();
        
        // Read the file line by line
        std::string line;
        std::string currentSection = "General"; // Default section
        
        while (std::getline(file, line)) {
            // Process the line
            parseLine(line, currentSection);
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Exception loading configuration: %s", ex.what());
        return false;
    }
}

bool ConfigManager::saveConfiguration()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try {
        // Open the config file for writing
        std::ofstream file(m_configPath);
        if (!file.is_open()) {
            return false;
        }
        
        // Write each section
        for (const auto& section : m_configData) {
            // Write section header
            file << "[" << section.first << "]" << std::endl;
            
            // Write key-value pairs
            for (const auto& keyValue : section.second) {
                file << keyValue.first << " = " << serializeValue(keyValue.second) << std::endl;
            }
            
            // Add an empty line between sections
            file << std::endl;
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        Logger::GetInstance()->error("Exception saving configuration: %s", ex.what());
        return false;
    }
}

std::string ConfigManager::getString(const std::string& section, const std::string& key, const std::string& defaultValue) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Look for the section
    auto sectionIt = m_configData.find(section);
    if (sectionIt == m_configData.end()) {
        return defaultValue;
    }
    
    // Look for the key
    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return defaultValue;
    }
    
    return keyIt->second;
}

int ConfigManager::getInt(const std::string& section, const std::string& key, int defaultValue) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Get the string value
    std::string strValue = getString(section, key, "");
    if (strValue.empty()) {
        return defaultValue;
    }
    
    // Try to convert to int
    try {
        return std::stoi(strValue);
    }
    catch (const std::exception&) {
        return defaultValue;
    }
}

double ConfigManager::getDouble(const std::string& section, const std::string& key, double defaultValue) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Get the string value
    std::string strValue = getString(section, key, "");
    if (strValue.empty()) {
        return defaultValue;
    }
    
    // Try to convert to double
    try {
        return std::stod(strValue);
    }
    catch (const std::exception&) {
        return defaultValue;
    }
}

bool ConfigManager::getBool(const std::string& section, const std::string& key, bool defaultValue) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Get the string value
    std::string strValue = getString(section, key, "");
    if (strValue.empty()) {
        return defaultValue;
    }
    
    // Convert to lowercase
    std::transform(strValue.begin(), strValue.end(), strValue.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // Check for true values
    if (strValue == "1" || strValue == "true" || strValue == "yes" || strValue == "on") {
        return true;
    }
    
    // Check for false values
    if (strValue == "0" || strValue == "false" || strValue == "no" || strValue == "off") {
        return false;
    }
    
    // Unknown value
    return defaultValue;
}

std::vector<std::string> ConfigManager::getStringList(const std::string& section, const std::string& key, const std::vector<std::string>& defaultValue) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Get the string value
    std::string strValue = getString(section, key, "");
    if (strValue.empty()) {
        return defaultValue;
    }
    
    // Parse the list
    std::vector<std::string> result;
    
    // Check if it's a JSON-style array
    if (strValue.front() == '[' && strValue.back() == ']') {
        // Remove the brackets
        strValue = strValue.substr(1, strValue.length() - 2);
        
        // Parse the comma-separated list
        std::istringstream ss(strValue);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Trim whitespace and quotes
            item = std::regex_replace(item, std::regex("^\\s+|\\s+$"), "");
            if (item.front() == '"' && item.back() == '"') {
                item = item.substr(1, item.length() - 2);
            }
            
            result.push_back(deserializeValue(item));
        }
    }
    else {
        // Treat as a single item
        result.push_back(strValue);
    }
    
    return result;
}

void ConfigManager::setString(const std::string& section, const std::string& key, const std::string& value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Set the value
    m_configData[section][key] = value;
}

void ConfigManager::setInt(const std::string& section, const std::string& key, int value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Convert to string and set
    m_configData[section][key] = std::to_string(value);
}

void ConfigManager::setDouble(const std::string& section, const std::string& key, double value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Convert to string with precision and set
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << value;
    m_configData[section][key] = ss.str();
}

void ConfigManager::setBool(const std::string& section, const std::string& key, bool value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Convert to string and set
    m_configData[section][key] = value ? "true" : "false";
}

void ConfigManager::setStringList(const std::string& section, const std::string& key, const std::vector<std::string>& value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Convert to JSON-style array
    std::ostringstream ss;
    ss << "[";
    
    for (size_t i = 0; i < value.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << "\"" << escapeString(value[i]) << "\"";
    }
    
    ss << "]";
    
    // Set the value
    m_configData[section][key] = ss.str();
}

bool ConfigManager::hasKey(const std::string& section, const std::string& key) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Look for the section
    auto sectionIt = m_configData.find(section);
    if (sectionIt == m_configData.end()) {
        return false;
    }
    
    // Look for the key
    return sectionIt->second.find(key) != sectionIt->second.end();
}

void ConfigManager::removeKey(const std::string& section, const std::string& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    // Look for the section
    auto sectionIt = m_configData.find(section);
    if (sectionIt == m_configData.end()) {
        return;
    }
    
    // Remove the key
    sectionIt->second.erase(key);
    
    // Remove the section if empty
    if (sectionIt->second.empty()) {
        m_configData.erase(sectionIt);
    }
}

ConfigManager* ConfigManager::GetInstance()
{
    std::call_once(s_onceFlag, []() {
        s_instance = std::make_unique<ConfigManager>();
    });
    
    return s_instance.get();
}

void ConfigManager::parseLine(const std::string& line, std::string& currentSection)
{
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#' || line[0] == ';') {
        return;
    }
    
    // Check for section
    if (line[0] == '[' && line.back() == ']') {
        currentSection = line.substr(1, line.size() - 2);
        return;
    }
    
    // Check for key-value pair
    size_t equalsPos = line.find('=');
    if (equalsPos != std::string::npos) {
        // Get the key and value
        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);
        
        // Trim whitespace
        key = std::regex_replace(key, std::regex("^\\s+|\\s+$"), "");
        value = std::regex_replace(value, std::regex("^\\s+|\\s+$"), "");
        
        // Set the value
        if (!key.empty()) {
            m_configData[currentSection][key] = deserializeValue(value);
        }
    }
}

std::string ConfigManager::serializeValue(const std::string& value) const
{
    return value;
}

std::string ConfigManager::deserializeValue(const std::string& value) const
{
    return value;
}

std::string ConfigManager::escapeString(const std::string& str) const
{
    std::string result;
    result.reserve(str.size());
    
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c;
        }
    }
    
    return result;
}

std::string ConfigManager::unescapeString(const std::string& str) const
{
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[i + 1]) {
                case '\\': result += '\\'; break;
                case '"':  result += '"'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += str[i + 1];
            }
            ++i;
        } else {
            result += str[i];
        }
    }
    
    return result;
}