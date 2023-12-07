/*
 * Copyright (c) 2005, Creative Labs Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 * 	     the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * 	     and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of Creative Labs Inc. nor the names of its contributors may be used to endorse or
 * 	     promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "stdafx.h"

#include "OpenALDeviceList.h"
#include "SoundRender_Core.h"

#include <al.h>
#include <alc.h>

#ifdef XR_PLATFORM_WINDOWS
constexpr pcstr AL_GENERIC_HARDWARE = "Generic Hardware";
constexpr pcstr AL_GENERIC_SOFTWARE = "Generic Software";
#endif

ALDeviceList::ALDeviceList()
{
    snd_device_id = (u32)-1;
    Enumerate();
}

void ALDeviceList::IterateAndAddDevicesString(pcstr devices)
{
    // go through device list (each device terminated with a single NULL, list terminated with double NULL)
    while (*devices != '\0')
    {
        if (ALCdevice* device = alcOpenDevice(devices))
        {
            if (ALCcontext* context = alcCreateContext(device, nullptr))
            {
                alcMakeContextCurrent(context);

                const bool enumerateAllPresent = alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT");

                // if new actual device name isn't already in the list, then add it...
                pcstr actualDeviceName = alcGetString(device, enumerateAllPresent ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER);

                if (actualDeviceName != nullptr && xr_strlen(actualDeviceName) > 0)
                {
                    int major, minor;
                    alcGetIntegerv(device, ALC_MAJOR_VERSION, sizeof(int), &major);
                    alcGetIntegerv(device, ALC_MINOR_VERSION, sizeof(int), &minor);

                    auto& addedDevice = m_devices.emplace_back(actualDeviceName, minor, major);
                }
                alcDestroyContext(context);
            }
            alcCloseDevice(device);
        }
        devices += xr_strlen(devices) + 1;
    }
}

void ALDeviceList::Enumerate()
{
#ifndef MASTER_GOLD
    Msg("SOUND: OpenAL: enumerate devices...");
#endif
    // have a set of vectors storing the device list, selection status, spec version #
    // -- empty all the lists and reserve space for 10 devices
    m_devices.clear();

    // grab function pointers for 1.1-API functions, and if successful proceed to enumerate all devices
    if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT"))
    {
        pcstr devices = (pstr)alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

        xr_strcpy(m_defaultDeviceName, alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER));
        Log("SOUND: OpenAL: system default sound device name is", m_defaultDeviceName);

        IterateAndAddDevicesString(devices);
    }
    // grab function pointers for 1.0-API functions, and if successful proceed to enumerate all devices
    else if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
    {
        pcstr devices = (pstr)alcGetString(nullptr, ALC_DEVICE_SPECIFIER);

        xr_strcpy(m_defaultDeviceName, alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER));
        Log("SOUND: OpenAL: system default sound device name is", m_defaultDeviceName);

#if defined(XR_PLATFORM_WINDOWS)
        // Xottab_DUTY
        // The problem from 2000s described below should not be relevant for Linux,
        // but still the case on Windows in 2022. And it probably won't be ever fixed...

        // ManowaR
        // "Generic Hardware" device on software AC'97 codecs introduce
        // high CPU usage ( up to 30% ) as a consequence - freezes, FPS drop
        // So if default device is "Generic Hardware" which maps to DirectSound3D interface
        // We re-assign it to "Generic Software" to get use of old good DirectSound interface
        // This makes 3D-sound processing unusable on cheap AC'97 codecs
        // Also we assume that if "Generic Hardware" exists, than "Generic Software" is also exists
        // Maybe wrong

        if (0 == xr_stricmp(m_defaultDeviceName, AL_GENERIC_HARDWARE))
        {
            xr_strcpy(m_defaultDeviceName, AL_GENERIC_SOFTWARE);
            Log("SOUND: OpenAL: default sound device name set to", m_defaultDeviceName);
        }
#endif

        IterateAndAddDevicesString(devices);
    }
    else
    {
        Msg("~ SOUND: OpenAL: EnumerationExtension NOT Present");
    }

    // make token
    const auto _cnt = GetNumDevices();

    auto& devices = SoundRender->Parent.GetDevicesList();
    devices.reserve(_cnt + 1);

    for (u32 i = 0; i < _cnt; ++i)
    {
        devices.emplace_back(xr_strdup(m_devices[i].name), i);
    }
    devices.emplace_back(nullptr, -1);
    //--

    if (0 == GetNumDevices())
    {
        Log("SOUND: OpenAL: No devices available.");
    }
    else
    {
#ifndef MASTER_GOLD
        Log("SOUND: OpenAL: All available devices:");
        int majorVersion, minorVersion;

        for (u32 j = 0; j < GetNumDevices(); j++)
        {
            GetDeviceVersion(j, &majorVersion, &minorVersion);
            Msg("%d. %s, Spec Version %d.%d %s", j + 1, GetDeviceName(j), majorVersion,
                minorVersion, xr_stricmp(GetDeviceName(j), m_defaultDeviceName) == 0 ? "(default)" : "");
        }
#endif
    }
}

pcstr ALDeviceList::GetDeviceName(size_t index) const
{
    return m_devices[index].name;
}

void ALDeviceList::SelectBestDevice()
{
    int best_majorVersion = -1;
    int best_minorVersion = -1;
    int majorVersion;
    int minorVersion;

    if (snd_device_id == (u32)-1)
    {
        // select best
        u32 new_device_id = snd_device_id;
        for (u32 i = 0; i < GetNumDevices(); ++i)
        {
            if (xr_stricmp(m_defaultDeviceName, GetDeviceName(i)) != 0)
                continue;

            GetDeviceVersion(i, &majorVersion, &minorVersion);
            if (majorVersion > best_majorVersion ||
                (majorVersion == best_majorVersion && minorVersion > best_minorVersion))
            {
                best_majorVersion = majorVersion;
                best_minorVersion = minorVersion;
                new_device_id = i;
            }
        }
        if (new_device_id == (u32)-1)
        {
            R_ASSERT(GetNumDevices() != 0);
            new_device_id = 0; // first
        }
        snd_device_id = new_device_id;
    }
    if (GetNumDevices() == 0)
        Msg("SOUND: Can't select device. List empty");
    else
        Msg("SOUND: Selected device is %s", GetDeviceName(snd_device_id));
}

/*
 * Returns the major and minor version numbers for a device at a specified index in the complete list
 */
void ALDeviceList::GetDeviceVersion(size_t index, int* major, int* minor)
{
    *major = m_devices[index].major_ver;
    *minor = m_devices[index].minor_ver;
}
