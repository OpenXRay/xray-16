#pragma once

inline int& Minimize1D::MaxLevel ()
{
    return m_iMaxLevel;
}

inline int& Minimize1D::MaxBracket ()
{
    return m_iMaxBracket;
}

inline void*& Minimize1D::UserData ()
{
    return m_pvUserData;
}
