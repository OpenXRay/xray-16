
#include "xiofile.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include <tchar.h>
#else
#define _tfopen fopen
#endif

bool CxIOFile::Open(LPCTSTR filename, LPCTSTR mode)
{
    if (m_fp) return false;	// Can't re-open without closing first

    m_fp = _tfopen(filename, mode);
    if (!m_fp) return false;

    m_bCloseFile = true;

    return true;
}

#if !defined(WIN32) && !defined(_WIN32_WCE)
#undef _tfopen
#endif
