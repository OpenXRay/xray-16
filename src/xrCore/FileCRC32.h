#pragma once

XRCORE_API void getFileCrc32(IReader* F, LPCSTR filePath, u32& outCrc, bool parseIncludes = true); // sets the value of outCrc
XRCORE_API void addFileCrc32(IReader* F, LPCSTR filePath, u32& outCrc, bool parseIncludes = true); // just adds to outCrc
