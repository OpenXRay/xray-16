//
// $Id: fcgio.cpp,v 1.13 2002/02/24 20:12:22 robs Exp $
//
// Allows you communicate with FastCGI streams using C++ iostreams
//
// ORIGINAL AUTHOR:     George Feinberg
// REWRITTEN BY:        Michael Richards  06/20/1999
// REWRITTEN AGAIN BY:  Michael Shell     02/23/2000
// REWRITTEN AGAIN BY:  Rob Saccoccio     11 Nov 2001
//
// Copyright (c) 2000 Tux the Linux Penguin
//
// You are free to use this software without charge or royalty
// as long as this notice is not removed or altered, and recognition
// is given to the author(s)
//
// This code is offered as-is without any warranty either expressed or
// implied; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)

#ifdef _WIN32
#define DLLAPI  __declspec(dllexport)
#endif

#include <limits.h>
#include "fcgio.h"

using std::streambuf;
using std::istream;
using std::ostream;
using std::streamsize;

fcgi_streambuf::fcgi_streambuf(FCGX_Stream * fs, char * b, int bs)
{
    init(fs, b, bs);
}
    
fcgi_streambuf::fcgi_streambuf(char_type * b, streamsize bs)
{
    init(0, b, bs);
}
    
fcgi_streambuf::fcgi_streambuf(FCGX_Stream * fs) 
{ 
    init(fs, 0, 0);
}

fcgi_streambuf::~fcgi_streambuf(void)
{
    overflow(EOF);
    // FCGX_Finish()/FCGX_Accept() will flush and close
}

void fcgi_streambuf::init(FCGX_Stream * fs, char_type * b, streamsize bs)
{
    this->fcgx = fs;
    this->buf = 0;
    this->bufsize = 0;
    setbuf(b, bs);    
}

int fcgi_streambuf::overflow(int c)
{
    if (this->bufsize)
    {
        int plen = pptr() - pbase();

        if (plen) 
        {
            if (FCGX_PutStr(pbase(), plen, this->fcgx) != plen) return EOF;
            pbump(-plen);
        }
    }

    if (c != EOF) 
    {
        if (FCGX_PutChar(c, this->fcgx) != c) return EOF;
    }

    return 0;
}

// default base class behaviour seems to be inconsistent
int fcgi_streambuf::sync()
{
    if (overflow(EOF)) return EOF;
    if (FCGX_FFlush(this->fcgx)) return EOF;
    return 0;
}

// uflow() removes the char, underflow() doesn't
int fcgi_streambuf::uflow() 
{
    int rv = underflow();
    if (this->bufsize) gbump(1);
    return rv;
}
				
// Note that the expected behaviour when there is no buffer varies
int fcgi_streambuf::underflow()
{
    if (this->bufsize)
    {
        if (in_avail() == 0)
        {
            int glen = FCGX_GetStr(eback(), this->bufsize, this->fcgx);
            if (glen <= 0) return EOF;

            setg(eback(), eback(), eback() + glen);
        }

        return (unsigned char) *gptr();       
    }
    else
    {
        return FCGX_GetChar(this->fcgx);
    } 
}

void fcgi_streambuf::reset(void)
{
    // it should be ok to set up both the get and put areas
    setg(this->buf, this->buf, this->buf);
    setp(this->buf, this->buf + this->bufsize);
}

std::streambuf * fcgi_streambuf::setbuf(char_type * b, streamsize bs)
{
    // XXX support moving data from an old buffer
    if (this->bufsize) return 0;

    this->buf = b;
    this->bufsize = bs;

    // the base setbuf() *has* to be called
    streambuf::setbuf(b, bs);

    reset();

    return this;
}

int fcgi_streambuf::attach(FCGX_Stream * fs)
{ 
    this->fcgx = fs;

    if (this->bufsize)
    {
        reset();
    }

    return 0;
}

streamsize fcgi_streambuf::xsgetn(char_type * s, streamsize n) 
{
    if (n > INT_MAX) return 0;
    return (this->bufsize) 
        ? streambuf::xsgetn(s, n) 
        : (streamsize) FCGX_GetStr((char *) s, (int) n, this->fcgx);
}
   
streamsize fcgi_streambuf::xsputn(const char_type * s, streamsize n) 
{
    if (n > INT_MAX) return 0;
    return (this->bufsize) 
        ? streambuf::xsputn(s, n) 
        : (streamsize) FCGX_PutStr((char *) s, (int) n, this->fcgx);
}

// deprecated
fcgi_istream::fcgi_istream(FCGX_Stream * fs) :
    istream(&fcgi_strmbuf)
{
    fcgi_strmbuf.attach(fs);
}

// deprecated
void fcgi_istream::attach(FCGX_Stream * fs)
{
    fcgi_strmbuf.attach(fs);
}

// deprecated
fcgi_ostream::fcgi_ostream(FCGX_Stream * fs) :
    ostream(&fcgi_strmbuf)
{
    fcgi_strmbuf.attach(fs);
}

// deprecated
void fcgi_ostream::attach(FCGX_Stream * fs)
{
    fcgi_strmbuf.attach(fs);
}

#pragma warning(pop)