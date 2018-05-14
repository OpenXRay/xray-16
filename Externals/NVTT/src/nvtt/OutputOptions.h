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

#ifndef NV_TT_OUTPUTOPTIONS_H
#define NV_TT_OUTPUTOPTIONS_H

#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
#include "nvtt.h"

namespace nvtt
{

	struct DefaultOutputHandler : public nvtt::OutputHandler
	{
		DefaultOutputHandler(const char * fileName) : stream(fileName) {}
		
		virtual ~DefaultOutputHandler()
		{
		}
		
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
		{
			// ignore.
		}
		
		// Output data.
		virtual bool writeData(const void * data, int size)
		{
			stream.serialize(const_cast<void *>(data), size);

			//return !stream.isError();
			return true;
		}
		
		nv::StdOutputStream stream;
	};
	
	
	struct OutputOptions::Private
	{
		nv::Path fileName;
		
		mutable OutputHandler * outputHandler;
		ErrorHandler * errorHandler;
		bool outputHeader;
		
		bool openFile() const;
		void closeFile() const;
	};

	
} // nvtt namespace


#endif // NV_TT_OUTPUTOPTIONS_H
