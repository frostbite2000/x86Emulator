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

#include "storage_device.h"

StorageDevice::StorageDevice(const std::string& name, DeviceType type)
    : Device(name),
      m_deviceType(type),
      m_mountedImagePath(""),
      m_active(false)
{
}

StorageDevice::~StorageDevice()
{
}

bool StorageDevice::isActive() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active;
}

StorageDevice::DeviceType StorageDevice::getDeviceType() const
{
    return m_deviceType;
}

std::string StorageDevice::getMountedImagePath() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mountedImagePath;
}

std::string StorageDevice::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}