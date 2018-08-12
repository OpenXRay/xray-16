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
#pragma hdrstop
#include "SDL.h"
#include "os_clipboard.h"
#include "xrCore/_std_extensions.h"

void os_clipboard::copy_to_clipboard(pcstr buf)
{
    if (SDL_SetClipboardText(buf) < 0)
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

    strncpy_s(buffer, buffer_size, clipData, buffer_size - 1);

    for (size_t i = 0; i < xr_strlen(buffer); ++i)
    {
        const char c = buffer[i];
        if (isprint(c) == 0 && c != char(-1) || c == '\t' || c == '\n') // "я" = -1
        {
            buffer[i] = ' ';
        }
    }

    SDL_free(clipData);
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
        DEBUG_BREAK;
        Msg("! Failed to get text from the clipboard: %s", SDL_GetError());
        Log("! Falling back to copy_to_clipboard()");
        copy_to_clipboard(string);
        return;
    }

    const size_t clipLength = xr_strlen(clipData);
    const size_t stringLength = xr_strlen(string);

    const size_t bufferSize = (clipLength + stringLength + 1) * sizeof(char);

    pstr buffer = (pstr)_alloca(bufferSize);

    xr_strcpy(buffer, bufferSize, clipData); // copy the clipboard
    xr_strcat(buffer, bufferSize, string);   // copy the new string

    SDL_free(clipData);

    copy_to_clipboard(buffer);
}
