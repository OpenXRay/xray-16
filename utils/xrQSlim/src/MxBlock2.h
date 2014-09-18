#ifndef MXBLOCK2_INCLUDED // -*- C++ -*-
#define MXBLOCK2_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  MxBlock2 provides typed access to 2D data blocks.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxBlock2.h,v 1.10.2.1 2004/07/01 18:38:41 garland Exp $

 ************************************************************************/

#include "MxBlock.h"

template<class T>
class MxBlock2 : public MxBlock<T>
{
private:
    int W, H;

protected:
    MxBlock2() { }

public:
    MxBlock2(int w, int h) : MxBlock<T>(w*h) { W=w; H=h; }

    T&       operator()(int i, int j)       { return (*this)[j*W+i]; }
    const T& operator()(int i, int j) const { return (*this)[j*W+i]; }

    int width() const { return W; }
    int height() const { return H; }

    void resize(int w, int h) { W=w; H=h; MxBlock<T>::resize_block(w*h); }
};

// MXBLOCK2_INCLUDED
#endif
