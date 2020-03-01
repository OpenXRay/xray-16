#include "stdafx.h"
#include "FileCRC32.h"

void getFileCrc32(IReader* F, LPCSTR filePath, u32& outCrc, bool parseIncludes)
{
    outCrc = crc32(F->pointer(), F->length(), outCrc);
    string4096 str;
    if (parseIncludes)
    {
        while (!F->eof())
        {
            F->r_string(str, sizeof str);
            _Trim(str);
            if (str[0] && _Trim(str)[0] == '#' && strstr(str, "#include")) // handle includes
            {
                string_path inc_name;
                R_ASSERT(filePath && filePath[0]);
                if (_GetItem(str, 1, inc_name, '"'))
                {
                    xr_strlwr(inc_name);
                    string_path fn;
                    strconcat(sizeof fn, fn, filePath, inc_name);
                    const xr_string inc_path = EFS_Utils::ExtractFilePath(fn);
                    IReader* I = FS.r_open(fn);
                    R_ASSERT3(I, "Can't find include file:", inc_name);
                    getFileCrc32(I, inc_path.c_str(), outCrc, true);
                    FS.r_close(I);
                }
            }
        }
    }
}
