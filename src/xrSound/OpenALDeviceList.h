#pragma once

#include "xrCore/_std_extensions.h"

struct ALDeviceDesc
{
    string256 name = { 0 };
    int minor_ver;
    int major_ver;
    union ESndProps
    {
        struct
        {
            u16 selected : 1;
            u16 eax : 3;
            u16 efx : 1;

            u16 unused : 9;
        };
        u16 storage;
    };
    ESndProps props;
    ALDeviceDesc(pcstr nm, int mn, int mj)
    {
        xr_strcpy(name, nm);
        minor_ver = mn;
        major_ver = mj;
        props.storage = 0;
    }
};

class ALDeviceList
{
    xr_vector<ALDeviceDesc> m_devices;
    xr_vector<ALDeviceDesc> m_capture_devices;
    string256 m_defaultDeviceName;
    string256 m_defaultCaptureDeviceName;

    void Enumerate();
    void IterateAndAddDevicesString(pcstr devices);
    void IterateAndAddCaptureDevicesString(pcstr devices);

public:
    ALDeviceList();

    [[nodiscard]]
    size_t GetNumDevices() const { return m_devices.size(); }

    [[nodiscard]]
    size_t GetNumCaptureDevices() const { return m_capture_devices.size(); }

    [[nodiscard]]
    const ALDeviceDesc& GetDeviceDesc(size_t index) const { return m_devices[index]; }

    [[nodiscard]]
    const ALDeviceDesc& GetCaptureDeviceDesc(size_t index) const { return m_capture_devices[index]; }

    [[nodiscard]]
    pcstr GetDeviceName(size_t index) const;

    [[nodiscard]]
    pcstr GetCaptureDeviceName(size_t index) const;

    void GetDeviceVersion(size_t index, int* major, int* minor);
    void GetCaptureDeviceVersion(size_t index, int* major, int* minor);

    void SelectBestDevice();
};
