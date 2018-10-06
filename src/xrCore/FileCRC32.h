#pragma once

XRCORE_API void getFileCrc32(IReader* F, LPCSTR filePath, u32& outCrc, bool parseIncludes = true);
