#include <windows.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "lwo2.h"
};

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{ 
    return TRUE;
}

extern "C"
{
    __declspec(dllexport) lwObject* LWOImportObject(char* filename)
    {
        unsigned int failID;
        int failpos;
        return lwGetObject(filename, &failID, &failpos);
    }

    __declspec(dllexport) void LWOCloseFile(lwObject* object)
    {
        lwFreeObject(object);
    }
}
