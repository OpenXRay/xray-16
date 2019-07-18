#pragma once

class AccessibilityShortcuts
{
    bool screensaverState;
    STICKYKEYS stickyKeys;
    FILTERKEYS filterKeys;
    TOGGLEKEYS toggleKeys;
    DWORD stickyKeysFlags;
    DWORD filterKeysFlags;
    DWORD toggleKeysFlags;

public:
    AccessibilityShortcuts()
    {
        screensaverState = false;
        stickyKeysFlags = 0;
        filterKeysFlags = 0;
        toggleKeysFlags = 0;
        stickyKeys = {};
        filterKeys = {};
        toggleKeys = {};
        stickyKeys.cbSize = sizeof(stickyKeys);
        filterKeys.cbSize = sizeof(filterKeys);
        toggleKeys.cbSize = sizeof(toggleKeys);
    }

    void Disable()
    {
        SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &screensaverState, 0);

        SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(stickyKeys), &stickyKeys, 0);
        SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(filterKeys), &filterKeys, 0);
        SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(toggleKeys), &toggleKeys, 0);

        if (screensaverState)
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, nullptr, 0);

        if (stickyKeys.dwFlags & SKF_AVAILABLE)
        {
            stickyKeysFlags = stickyKeys.dwFlags;
            stickyKeys.dwFlags = 0;
            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(stickyKeys), &stickyKeys, 0);
        }

        if (filterKeys.dwFlags & FKF_AVAILABLE)
        {
            filterKeysFlags = filterKeys.dwFlags;
            filterKeys.dwFlags = 0;
            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(filterKeys), &filterKeys, 0);
        }

        if (toggleKeys.dwFlags & TKF_AVAILABLE)
        {
            toggleKeysFlags = toggleKeys.dwFlags;
            toggleKeys.dwFlags = 0;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(toggleKeys), &toggleKeys, 0);
        }
    }

    ~AccessibilityShortcuts()
    {
        if (screensaverState)
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, nullptr, 0);

        if (stickyKeysFlags)
        {
            stickyKeys.dwFlags = stickyKeysFlags;
            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(stickyKeys), &stickyKeys, 0);
        }

        if (filterKeysFlags)
        {
            filterKeys.dwFlags = filterKeysFlags;
            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(filterKeys), &filterKeys, 0);
        }

        if (toggleKeysFlags)
        {
            toggleKeys.dwFlags = toggleKeysFlags;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(toggleKeys), &toggleKeys, 0);
        }
    }
};
