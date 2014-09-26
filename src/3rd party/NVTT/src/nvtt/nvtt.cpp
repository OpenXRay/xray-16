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

#include "nvtt.h"

using namespace nvtt;

/// Return a string for the given error.
const char * nvtt::errorString(Error e)
{
	switch(e)
	{
		case Error_Unknown:
			return "Unknown error";
		case Error_InvalidInput:
			return "Invalid input";
		case Error_UnsupportedFeature:
			return "Unsupported feature";
		case Error_CudaError:
			return "CUDA error";
		case Error_FileOpen:
			return "Error opening file";
		case Error_FileWrite:
			return "Error writing through output handler";
	}
	
	return "Invalid error";
}

/// Return NVTT version.
unsigned int nvtt::version()
{
	return NVTT_VERSION;
}

