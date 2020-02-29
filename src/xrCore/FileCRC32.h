#pragma once

XRCORE_API void getFileCrc32(IReader* F, const char* filePath, u32& outCrc, bool parseIncludes = true);
