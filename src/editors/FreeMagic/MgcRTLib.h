// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

#ifndef MGCRTLIB_H
#define MGCRTLIB_H

// A wrapper around some headers for run-time libraries.  I added this because
// CodeWarrior 6.1 for the Macintosh uses namespace std for functions exposed
// in cmath, cstring, etc.  Windows or Linux does not do this.  I do not know
// what the STL standard is for this.

#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__MACOS__) && !defined(__MACH__)
using namespace std;
#endif

#endif
