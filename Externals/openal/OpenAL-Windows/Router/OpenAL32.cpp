/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */




#include <windows.h>
#include "OpenAL32.h"


//*****************************************************************************
// DllMain
//*****************************************************************************
//
BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID reserved)
{
    BOOL result = TRUE;

    // Perform actions based on the reason for calling.
    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            // Create the context list lock so I can safely add/remove contexts.
            result = alListCreate(&alContextList);
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            // Perform any necessary cleanup.
            alListFree(alContextList);
            alContextList = 0;
            break;
    }

    return TRUE;
}

