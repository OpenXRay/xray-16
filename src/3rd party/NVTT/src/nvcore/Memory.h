// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_MEMORY_H
#define NV_CORE_MEMORY_H

#include <nvcore/nvcore.h>

#include <stdlib.h> // malloc(), realloc() and free()
#include <stddef.h>	// size_t

#include <new>	// new and delete

// Custom memory allocator
namespace nv
{
	namespace mem 
	{
		NVCORE_API void * malloc(size_t size);
		NVCORE_API void * malloc(size_t size, const char * file, int line);
		
		NVCORE_API void free(const void * ptr);
		NVCORE_API void * realloc(void * ptr, size_t size);
		
	} // mem namespace
	
} // nv namespace


// Override new/delete

inline void * operator new (size_t size) throw()
{
	return nv::mem::malloc(size); 
}

inline void operator delete (void *p) throw()
{
	nv::mem::free(p); 
}

inline void * operator new [] (size_t size) throw()
{
	return nv::mem::malloc(size);
}

inline void operator delete [] (void * p) throw()
{
	nv::mem::free(p); 
}

/*
#ifdef _DEBUG
#define new new(__FILE__, __LINE__)
#define malloc(i) malloc(i, __FILE__, __LINE__)
#endif
*/

#if 0
/*
    File:	main.cpp
    
    Version:	1.0

	Abstract: Overrides the C++ 'operator new' and 'operator delete'.

    Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
		("Apple") in consideration of your agreement to the following terms, and your
		use, installation, modification or redistribution of this Apple software
		constitutes acceptance of these terms.  If you do not agree with these terms,
		please do not use, install, modify or redistribute this Apple software.

		In consideration of your agreement to abide by the following terms, and subject
		to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
		copyrights in this original Apple software (the "Apple Software"), to use,
		reproduce, modify and redistribute the Apple Software, with or without
		modifications, in source and/or binary forms; provided that if you redistribute
		the Apple Software in its entirety and without modifications, you must retain
		this notice and the following text and disclaimers in all such redistributions of
		the Apple Software.  Neither the name, trademarks, service marks or logos of
		Apple Computer, Inc. may be used to endorse or promote products derived from the
		Apple Software without specific prior written permission from Apple.  Except as
		expressly stated in this notice, no other rights or licenses, express or implied,
		are granted by Apple herein, including but not limited to any patent rights that
		may be infringed by your derivative works or by other works in which the Apple
		Software may be incorporated.

		The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
		WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
		WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
		PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
		COMBINATION WITH YOUR PRODUCTS.

		IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
		CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
		GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
		ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
		OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
		(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
		ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Copyright © 2006 Apple Computer, Inc., All Rights Reserved
*/

/* This sample shows how to override the C++ global 'new' and 'delete' operators.  */
#include <new>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <locale>

/* Some variables and code to make the example do something.  */
namespace {
  unsigned long long gNewCounter; // number of times 'new' was called
  unsigned long long gDeleteCounter;  // number of times 'delete' was called
  
  void printCounters()  // print the counters above
  {
	std::cout << "new was called " << gNewCounter << " times and delete was called " << gDeleteCounter << " times\n";
  }
}

/* These are the overridden new and delete routines.
   Most applications will want to override at least these four versions of new/delete if they override any of them.

   In Mac OS, it's not necessary to override the array versions of operator new and delete if all
   they would do is call the non-array versions; the C++ standard library, as an extension
   to the C++ standard, does this for you.

   Developers should consult the section [lib.support.dynamic] in the C++ standard to see the requirements
   on the generic operators new and delete; the system may expect that your overridden operators meet all these
   requirements.

   Your operators may be called by the system, even early in start-up before constructors have been executed.  */
void* operator new(std::size_t sz) throw (std::bad_alloc)
{
	void *result = std::malloc (sz == 0 ? 1 : sz);
	if (result == NULL)
		throw std::bad_alloc();
	gNewCounter++;
	return result;
}
void operator delete(void* p) throw()
{
	if (p == NULL)
		return;
	std::free (p);
	gDeleteCounter++;
}

/* These are the 'nothrow' versions of the above operators.
   The system version will try to call a std::new_handler if they
   fail, but your overriding versions are not required to do this.  */
void* operator new(std::size_t sz, const std::nothrow_t&) throw()
{
	try {
		void * result = ::operator new (sz);  // calls our overridden operator new
		return result;
	} catch (std::bad_alloc &) {
	  return NULL;
	}
}
void operator delete(void* p, const std::nothrow_t&) throw()
{
	::operator delete (p);
}

/* Bug 4067110 is that if your program has no weak symbols at all, the linker will not set the
   WEAK_DEFINES bit in the Mach-O header and as a result the new and delete operators above won't
   be seen by system libraries.  This is mostly a problem for test programs and small examples,
   since almost all real C++ programs complicated enough to override new and delete will have at
   least one weak symbol.  However, this is a small example, so:  */
void __attribute__((weak, visibility("default"))) workaroundFor4067110 () { }

/* This is a simple test program that causes the runtime library to call new and delete.  */
int main() 
{
	atexit (printCounters);
	try {
	  std::locale example("does_not_exist");
	} catch (std::runtime_error &x) {
	}
	return 0;
}
#endif // 0

#endif // NV_CORE_MEMORY_H
