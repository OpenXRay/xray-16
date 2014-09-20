/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: .NET symbolic engine.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "SymEngineNet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _MANAGED

#pragma managed

namespace IntelleSoft
{
	namespace BugTrap
	{
		StackFrame^ StackFrameEnumerator::GetNextStackTraceEntry(void)
		{
			return (this->frameIndex < this->stackTrace->FrameCount ? this->stackTrace->GetFrame(this->frameIndex++) : nullptr);
		}

		AssemblyEnumerator::AssemblyEnumerator(void)
		{
			AppDomain^ domain = AppDomain::CurrentDomain;
			this->assemblies = domain->GetAssemblies();
			this->assemblyEnumerator = this->assemblies->GetEnumerator();
		}

		Assembly^ AssemblyEnumerator::GetNextAssembly(void)
		{
			return (this->assemblyEnumerator != nullptr && this->assemblyEnumerator->MoveNext() ? safe_cast<Assembly^>(this->assemblyEnumerator->Current) : nullptr);
		}
	}
}

#pragma unmanaged

#endif // _MANAGED

