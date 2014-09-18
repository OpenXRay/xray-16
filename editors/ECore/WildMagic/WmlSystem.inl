// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

//----------------------------------------------------------------------------
template <class T>
void Allocate2D (int iCols, int iRows, T**& raatArray)
{
    raatArray = new T*[iRows];
    raatArray[0] = new T[iRows*iCols];
    for (int iRow = 1; iRow < iRows; iRow++)
        raatArray[iRow] = &raatArray[0][iCols*iRow];
}
//----------------------------------------------------------------------------
template <class T>
void Deallocate2D (T** aatArray)
{
    delete[] aatArray[0];
    delete[] aatArray;
}
//----------------------------------------------------------------------------
template <class T>
void Allocate3D (int iCols, int iRows, int iSlices, T***& raaatArray)
{
    raaatArray = new T**[iSlices];
    for (int iSlice = 0; iSlice < iSlices; iSlice++)
    {
        raaatArray[iSlice] = new T*[iRows];
        for (int iRow = 0; iRow < iRows; iRow++)
            raaatArray[iSlice][iRow] = new T[iCols];
    }
}
//----------------------------------------------------------------------------
template <class T>
void Deallocate3D (int iRows, int iSlices, T*** aaatArray)
{
    for (int iSlice = 0; iSlice < iSlices; iSlice++)
    {
        for (int iRow = 0; iRow < iRows; iRow++)
            delete[] aaatArray[iSlice][iRow];
        delete[] aaatArray[iSlice];
    }
    delete[] aaatArray;
}
//----------------------------------------------------------------------------
