/* [SCE CONFIDENTIAL DOCUMENT]
 * PLAYSTATION(R)3 SPU Optimized Bullet Physics Library (http://bulletphysics.com)
 *                Copyright (C) 2007 Sony Computer Entertainment Inc.
 *                                                All Rights Reserved.
 */

#ifndef __CELL_CONFIGURATION_H
#define __CELL_CONFIGURATION_H

#undef SCE_CONTROL_CONSOLE

#ifdef WIN32
#define EXPORT_SYM __declspec( dllexport )
#else
#define EXPORT_SYM 
#endif

#endif
