/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Text format analyzer.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "Encoding.h"

/// UTF-8 preamble.
const BYTE g_arrUTF8Preamble[] = { 0xEF, 0xBB, 0xBF };
/// UTF-16/UCS-2, little-endian preamble.
const BYTE g_arrUTF16LEPreamble[] = { 0xFF, 0xFE };
/// UTF-16/UCS-2, big-endian preamble.
const BYTE g_arrUTF16BEPreamble[] = { 0xFE, 0xFF };

DWORD DetectFileFormat(PCTSTR pszFileName, HANDLE hFile, BOOL& bTextFile, TEXT_ENCODING& eEncoding);
