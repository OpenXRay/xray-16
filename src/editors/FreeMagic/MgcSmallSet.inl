// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

//----------------------------------------------------------------------------
template <class T>
SmallSet<T>::SmallSet()
{
    m_iCapacity = 1;
    m_iGrowBy = 1;
    m_iSize = 0;
    m_atElement = new T[1];
}
//----------------------------------------------------------------------------
template <class T>
SmallSet<T>::SmallSet(int iCapacity, int iGrowBy)
{
    assert(iCapacity > 0 && iGrowBy > 0);

    m_iCapacity = iCapacity;
    m_iGrowBy = iGrowBy;
    m_iSize = 0;
    m_atElement = new T[iCapacity];
}
//----------------------------------------------------------------------------
template <class T>
SmallSet<T>::SmallSet(const SmallSet &rkSet)
{
    m_iCapacity = rkSet.m_iCapacity;
    m_iGrowBy = rkSet.m_iGrowBy;
    m_iSize = rkSet.m_iSize;
    m_atElement = new T[m_iCapacity];
    memcpy(m_atElement, rkSet.m_atElement, m_iCapacity * sizeof(T));
}
//----------------------------------------------------------------------------
template <class T>
SmallSet<T>::~SmallSet()
{
    delete[] m_atElement;
}
//----------------------------------------------------------------------------
template <class T>
SmallSet<T> &SmallSet<T>::operator=(const SmallSet &rkSet)
{
    delete[] m_atElement;
    m_iCapacity = rkSet.m_iCapacity;
    m_iGrowBy = rkSet.m_iGrowBy;
    m_iSize = rkSet.m_iSize;
    m_atElement = new T[m_iCapacity];
    memcpy(m_atElement, rkSet.m_atElement, m_iCapacity * sizeof(T));
    return *this;
}
//----------------------------------------------------------------------------
template <class T>
int SmallSet<T>::GetCapacity() const
{
    return m_iCapacity;
}
//----------------------------------------------------------------------------
template <class T>
int SmallSet<T>::GetGrowBy() const
{
    return m_iGrowBy;
}
//----------------------------------------------------------------------------
template <class T>
int SmallSet<T>::GetSize() const
{
    return m_iSize;
}
//----------------------------------------------------------------------------
template <class T>
const T *SmallSet<T>::GetElements() const
{
    return m_atElement;
}
//----------------------------------------------------------------------------
template <class T>
const T &SmallSet<T>::operator[](int i) const
{
    assert(0 <= i && i < m_iSize);
    return m_atElement[i];
}
//----------------------------------------------------------------------------
template <class T>
bool SmallSet<T>::Insert(const T &rkElement)
{
    for (int i = 0; i < m_iSize; i++)
    {
        if (rkElement == m_atElement[i])
            return false;
    }

    if (m_iSize == m_iCapacity)
    {
        // array is full, resize it
        int iNewCapacity = m_iCapacity + m_iGrowBy;
        T *atNewElement = new T[iNewCapacity];
        memcpy(atNewElement, m_atElement, m_iCapacity * sizeof(T));
        delete[] m_atElement;
        m_atElement = atNewElement;
        m_iCapacity = iNewCapacity;
    }

    m_atElement[m_iSize++] = rkElement;
    return true;
}
//----------------------------------------------------------------------------
template <class T>
void SmallSet<T>::InsertNoCheck(const T &rkElement)
{
    if (m_iSize == m_iCapacity)
    {
        // array is full, resize it
        int iNewCapacity = m_iCapacity + m_iGrowBy;
        T *atNewElement = new T[iNewCapacity];
        memcpy(atNewElement, m_atElement, m_iCapacity * sizeof(T));
        delete[] m_atElement;
        m_atElement = atNewElement;
        m_iCapacity = iNewCapacity;
    }

    m_atElement[m_iSize++] = rkElement;
}
//----------------------------------------------------------------------------
template <class T>
bool SmallSet<T>::Remove(const T &rkElement)
{
    for (int i = 0; i < m_iSize; i++)
    {
        if (rkElement == m_atElement[i])
        {
            // element exists, shift array to fill in empty slot
            for (int j = i + 1; j < m_iSize; j++, i++)
                m_atElement[i] = m_atElement[j];

            m_iSize--;
            return true;
        }
    }

    return false;
}
//----------------------------------------------------------------------------
template <class T>
bool SmallSet<T>::Exists(const T &rkElement)
{
    for (int i = 0; i < m_iSize; i++)
    {
        if (rkElement == m_atElement[i])
            return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class T>
void SmallSet<T>::Clear(int iCapacity, int iGrowBy)
{
    assert(iCapacity > 0 && iGrowBy > 0);

    delete[] m_atElement;
    m_iCapacity = iCapacity;
    m_iGrowBy = iGrowBy;
    m_iSize = 0;
    m_atElement = new T[iCapacity];
}
//----------------------------------------------------------------------------
