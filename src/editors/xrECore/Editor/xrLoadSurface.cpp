#include "stdafx.h"
#pragma hdrstop

//#include "freeimage/freeimage.h"

struct SExts
{
  xr_vector<LPSTR> exts;
  void format_register(LPCSTR ext)
  {
    if (ext && ext[0])
    {
      for (u32 i = 0; i < exts.size(); i++)
        if (0 == stricmp(exts[i], ext))
          return;
      exts.push_back(xr_strdup(ext));
    }
  }
  u32 size() { return (u32)exts.size(); }
  LPSTR operator[](int k) { return exts[k]; }
  ~SExts()
  {
    for (u32 i = 0; i < exts.size(); i++)
      xr_free(exts[i]);
    exts.clear();
  }
};
SExts formats;
