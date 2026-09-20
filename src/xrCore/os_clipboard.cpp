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
#include "Text/Utf8Utils.hpp"

#include <SDL.h>

void os_clipboard::copy_to_clipboard(pcstr buf, bool /*alreadyUTF8*/ /*= true*/)
{
    // All internal strings are UTF-8 now, so pass them directly to SDL.
    const int result = SDL_SetClipboardText(buf);
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

    strncpy_s(buffer, buffer_size, clipData, buffer_size - 1);
    SDL_free(clipData);

    // If strncpy_s truncated inside a multi-byte UTF-8 sequence, the buffer ends
    // with a stray continuation byte of an incomplete codepoint. Trim only that
    // incomplete tail back to its leading byte. NOTE: we must NOT trim every
    // trailing continuation byte — a completed multi-byte codepoint (e.g. Cyrillic
    // 'т' = D1 82) also ends in a continuation byte and must be preserved.
    const size_t pastedLength = xr_strlen(buffer);
    if (pastedLength > 0 && XRay::Utf8::IsContinuationByte(static_cast<u8>(buffer[pastedLength - 1])))
    {
        pcstr leading = XRay::Utf8::Prev(buffer, buffer + pastedLength);
        const size_t leadingIdx = leading - buffer;
        const size_t seqLen = XRay::Utf8::SequenceLength(static_cast<u8>(*leading));
        // Trim only when the leading byte's sequence is actually cut off.
        if (seqLen > 0 && leadingIdx + seqLen > pastedLength)
            buffer[leadingIdx] = '\0';
    }

    const size_t length = xr_strlen(buffer);
    for (size_t i = 0; i < length; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(buffer[i]);
        // Allow printable ASCII, UTF-8 continuation bytes and UTF-8 leading bytes.
        // Only reject control characters except tab and newline.
        if (c < 0x20 && c != '\t' && c != '\n')
            buffer[i] = ' ';
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

    const size_t clipLength = xr_strlen(clipData);
    const size_t stringLength = xr_strlen(string);

    const size_t bufferSize = (clipLength + stringLength + 1) * sizeof(char);

    pstr buffer = (pstr)xr_alloca(bufferSize);

    xr_strcpy(buffer, bufferSize, clipData); // copy the clipboard
    xr_strcat(buffer, bufferSize, string); // copy the new string

    SDL_free(clipData);

    copy_to_clipboard(buffer);
}
