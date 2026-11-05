///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Main file for Opcode.dll.
 *	\file		Opcode.cpp
 *	\author		Pierre Terdiman
 *	\date		March, 20, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	Finding a good name is difficult!
	Here's the draft for this lib.... Spooky, uh?

	VOID?			Very Optimized Interference Detection
	ZOID?			Zappy's Optimized Interference Detection
	CID?			Custom/Clever Interference Detection
	AID / ACID!		Accurate Interference Detection
	QUID?			Quick Interference Detection
	RIDE?			Realtime Interference DEtection
	WIDE?			Wicked Interference DEtection (....)
	GUID!
	KID !			k-dop interference detection :)
	OPCODE!			OPtimized COllision DEtection
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "Stdafx.h"
#include <fstream>
#include <iostream>
//std::ofstream gFile;

#include <cstdarg>

void Opcode_Log (const char* msg, ...)
{
	//va_list args;
	//va_start (args, msg);
    //char mesg[256];
	//sprintf(mesg, msg, args);
	//gFile << mesg;
	//gFile.flush();
	//va_end (args);
}

bool Opcode_Err (const char* msg, ...)
{
	//va_list args;
	//va_start (args, msg);
	//// Although it's called "..._Err", Opcode also reports less-than-fatal
	//// messages through it

    //char mesg[256];
	//sprintf(mesg, msg, args);
	//gFile << mesg;
	//gFile.flush();
	//va_end (args);
	return false;
}

bool Opcode::InitOpcode()
{
	//gFile.open("opcode.log");
	//Log("// Initializing OPCODE");
	//LogAPIInfo();
	return true;
}

void ReleasePruningSorters();
bool Opcode::CloseOpcode()
{
	//Log("// Closing OPCODE");

	ReleasePruningSorters();

	return true;
}

#ifdef ICE_MAIN

void ModuleAttach(HINSTANCE hinstance)
{
}

void ModuleDetach()
{
}

#endif
