#pragma once

#include "xrCore/_std_extensions.h"

struct ALDeviceDesc
{
    string256 name = { 0 };
    int minor_ver;
    int major_ver;
    struct ESndProps
    {
        bool selected : 1;
    };

    ESndProps props{};
    ALDeviceDesc(pcstr nm, int mn, int mj)
    {
        xr_strcpy(name, nm);
        minor_ver = mn;
        major_ver = mj;
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

    [[nodiscard]]
    size_t GetNumDevices() const { return m_devices.size(); }

    [[nodiscard]]
    const ALDeviceDesc& GetDeviceDesc(size_t index) const { return m_devices[index]; }

    [[nodiscard]]
    pcstr GetDeviceName(size_t index) const;

    void GetDeviceVersion(size_t index, int* major, int* minor);

    void SelectBestDevice();
};
