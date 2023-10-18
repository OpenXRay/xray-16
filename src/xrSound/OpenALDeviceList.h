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
    string256 m_defaultDeviceName;

    void Enumerate();
    void IterateAndAddDevicesString(pcstr devices);

public:
    ALDeviceList();
    ~ALDeviceList();

    [[nodiscard]]
    size_t GetNumDevices() const { return m_devices.size(); }

    [[nodiscard]]
    const ALDeviceDesc& GetDeviceDesc(size_t index) const { return m_devices[index]; }

    [[nodiscard]]
    pcstr GetDeviceName(size_t index) const;

    void GetDeviceVersion(size_t index, int* major, int* minor);

    void SelectBestDevice();
};
