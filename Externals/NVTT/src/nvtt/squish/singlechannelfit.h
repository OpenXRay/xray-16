/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2006 Ignacio Castano                      castanyo@yahoo.es

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#ifndef SQUISH_SINGLECHANNELFIT_H
#define SQUISH_SINGLECHANNELFIT_H

#include <squish.h>
#include "maths.h"
#include "colourfit.h"

namespace squish {

class SingleChannelFit : public ColourFit
{
public:
	SingleChannelFit( ColourSet const* colours, int flags );
	
private:
	virtual void Compress3( void* block );
	virtual void Compress4( void* block );

private:
	u8 m_greys[16];
	int m_g_min;
	int m_g_max;
};

} // namespace squish

#endif // ndef SQUISH_SINGLECHANNELFIT_H
