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

extern "C"{ 
	__declspec(dllexport) lwObject* LWO_ImportObject(char* filename, lwObject* new_obj){
		unsigned int failID;
		int failpos;
		new_obj = lwGetObject( filename, &failID, &failpos );
		return new_obj; 
	}

	__declspec(dllexport) void LWO_CloseFile(lwObject *new_obj){
		lwFreeObject( new_obj );
	}
}

