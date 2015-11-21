// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef CMDLINE_H
#define CMDLINE_H

#include <nvcore/Debug.h>

#include <stdio.h> // stderr
#include <stdlib.h>	// exit
#include <stdarg.h> // va_list


struct MyMessageHandler : public nv::MessageHandler {
	MyMessageHandler() {
		nv::debug::setMessageHandler( this );
	}
	~MyMessageHandler() {
		nv::debug::resetMessageHandler();
	}

	virtual void log( const char * str, va_list arg ) {
		va_list val;
		va_copy(val, arg);
		vfprintf(stderr, str, arg);
		va_end(val);		
	}
};


struct MyAssertHandler : public nv::AssertHandler {
	MyAssertHandler() {
		nv::debug::setAssertHandler( this );
	}
	~MyAssertHandler() {
		nv::debug::resetAssertHandler();
	}
	
	// Handler method, note that func might be NULL!
	virtual int assert( const char *exp, const char *file, int line, const char *func ) {
		fprintf(stderr, "Assertion failed: %s\nIn %s:%d\n", exp, file, line);
		nv::debug::dumpInfo();
		exit(1);
	}
};


#endif // CMDLINE_H
