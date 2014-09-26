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

#include "OutputOptions.h"

using namespace nvtt;


OutputOptions::OutputOptions() : m(*new OutputOptions::Private())
{
	reset();
}

OutputOptions::~OutputOptions()
{
	delete &m;
}

/// Set default output options.
void OutputOptions::reset()
{
	m.fileName.reset();
	m.outputHandler = NULL;
	m.errorHandler = NULL;
	m.outputHeader = true;
}


/// Set output file name.
void OutputOptions::setFileName(const char * fileName)
{
	m.fileName = fileName;
	m.outputHandler = NULL;
}

/// Set output handler.
void OutputOptions::setOutputHandler(OutputHandler * outputHandler)
{
	m.fileName.reset();
	m.outputHandler = outputHandler;
}

/// Set error handler.
void OutputOptions::setErrorHandler(ErrorHandler * errorHandler)
{
	m.errorHandler = errorHandler;
}

/// Set output header.
void OutputOptions::setOutputHeader(bool outputHeader)
{
	m.outputHeader = outputHeader;
}


bool OutputOptions::Private::openFile() const
{
	if (!fileName.isNull())
	{
		nvCheck(outputHandler == NULL);
		
		DefaultOutputHandler * oh = new DefaultOutputHandler(fileName.str());
		if (oh->stream.isError())
		{
			return false;
		}
		
		outputHandler = oh;
	}
	
	return true;
}

void OutputOptions::Private::closeFile() const
{
	if (!fileName.isNull())
	{
		delete outputHandler;
		outputHandler = NULL;
	}
}

