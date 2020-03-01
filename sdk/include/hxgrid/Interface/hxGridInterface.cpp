#include "hxGridInterface.h"
#include "assert.h"

typedef IGridUser* (__cdecl TCreateGridUserObject)(DWORD version);

typedef IGenericStream* (__cdecl TCreateGenericStream)();

//==============================================================
//==============================================================
IGridUser* CreateGridUserObject(DWORD version)
{
 static HINSTANCE DLLHandle(0);
 
 if (DLLHandle==0)
  {
   DLLHandle = LoadLibrary("hxGridUserDLL.dll");
   assert(DLLHandle!=0);
  } 
  
 static TCreateGridUserObject* pProc(NULL); 

 if (pProc==NULL)
  {
   pProc = (TCreateGridUserObject*)GetProcAddress(DLLHandle,"CreateGridUserObject");
   assert(pProc!=NULL);
  }
 return pProc(version); 
}

//==============================================================
//==============================================================
IGenericStream* CreateGenericStream()
{
 static HINSTANCE DLLHandle(0);
 
 if (DLLHandle==0)
  {
   DLLHandle = LoadLibrary("hxGridUserDLL.dll");
   assert(DLLHandle!=0);
  } 
  
 static TCreateGenericStream* pProc(NULL); 

 if (pProc==NULL)
  {
   pProc = (TCreateGenericStream*)GetProcAddress(DLLHandle,"CreateGenericStream");
   assert(pProc!=NULL);
  }
 return pProc(); 
}
