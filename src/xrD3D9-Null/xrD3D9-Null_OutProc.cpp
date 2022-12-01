#include "stdafx.h"

#include <tuple>
#include <stdio.h>

void LogOut(const char* format, ...)
{
    constexpr size_t BUF_SIZE = 4096;
    char text[BUF_SIZE];

    va_list argptr;
    va_start(argptr, format);
    vsprintf_s(text, BUF_SIZE, format, argptr);
    va_end(argptr);

    // rr  printf(text);
    //	OutputDebugString( text );
}

void LogOut_File(const char* pszFormat, ...)
{
    constexpr size_t BUF_SIZE = 128;
    char s[BUF_SIZE];
    va_list va;
    va_start(va, pszFormat);
    vsprintf_s(s, BUF_SIZE, pszFormat, va);
    va_end(va);
    fputs(s, stderr);

    FILE* fp;
    std::ignore = fopen_s(&fp, "d3d9-null.log", "a+t");
    if (fp)
    {
        fprintf(fp, "%s", s);
        fclose(fp);
    }
}
