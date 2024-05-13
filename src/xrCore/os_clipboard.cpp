////////////////////////////////////////////////////////////////////////////
// Module : os_clipboard.cpp
// Created : 21.02.2008
// Author : Evgeniy Sokolov
// Description : os clipboard class implementation
//
// Modified : 24.07.2018
// Modified by : Xottab_DUTY
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "os_clipboard.h"
#include "Text/StringConversion.hpp"

#include <SDL3/SDL.h>

#include <locale>

void os_clipboard::copy_to_clipboard(pcstr buf, bool alreadyUTF8 /*= false*/)
{
    int result;
    if (alreadyUTF8)
    {
        result = SDL_SetClipboardText(buf);
    }
    else
    {
        static std::locale locale("");
        xr_string string = StringToUTF8(buf, locale);
        result = SDL_SetClipboardText(string.c_str());
    }
    if (result < 0)
    {
        Msg("! Failed to copy text to the clipboard: %s", SDL_GetError());
        Log(buf);
    }
}

void os_clipboard::paste_from_clipboard(pstr buffer, size_t buffer_size)
{
    VERIFY(buffer);
    VERIFY(buffer_size > 0);

    if (!SDL_HasClipboardText())
        return;

    char* clipData = SDL_GetClipboardText();

    if (!clipData)
    {
        Msg("! Failed to paste text from the clipboard: %s", SDL_GetError());
        return;
    }

    static std::locale locale("");
    const xr_string string = StringFromUTF8(clipData, locale);
    SDL_free(clipData);

    strncpy_s(buffer, buffer_size, string.c_str(), buffer_size - 1);

    const size_t length = xr_strlen(buffer);
    for (size_t i = 0; i < length; ++i)
    {
        const char c = buffer[i];
        if ((std::isprint(c, locale) == 0 && c != char(-1)) || c == '\t' || c == '\n') // "я" = -1
        {
            buffer[i] = ' ';
        }
    }
}

void os_clipboard::update_clipboard(pcstr string)
{
    if (!string)
    {
        Log("! Why are you trying to copy nullptr to the clipboard?!");
        return;
    }

    if (!SDL_HasClipboardText())
    {
        copy_to_clipboard(string);
        return;
    }

    char* clipData = SDL_GetClipboardText();

    if (!clipData)
    {
        VERIFY3(clipData, "Failed to get text from the clipboard", SDL_GetError());
        copy_to_clipboard(string);
        return;
    }

    static std::locale locale("");
    const xr_string stringInUTF8 = StringToUTF8(string, locale);

    const size_t clipLength = xr_strlen(clipData);
    const size_t stringLength = stringInUTF8.size();

    const size_t bufferSize = (clipLength + stringLength + 1) * sizeof(char);

    pstr buffer = (pstr)xr_alloca(bufferSize);

    xr_strcpy(buffer, bufferSize, clipData); // copy the clipboard
    xr_strcat(buffer, bufferSize, stringInUTF8.c_str()); // copy the new string

    SDL_free(clipData);

    copy_to_clipboard(buffer, true);
}
