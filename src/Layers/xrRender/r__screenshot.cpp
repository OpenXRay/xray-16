#include "stdafx.h"

void CRender::ScreenshotAsyncBegin()
{
    VERIFY(!m_bMakeAsyncSS);
    m_bMakeAsyncSS = true;
}

void DoAsyncScreenshot() { RImplementation.Target->DoAsyncScreenshot(); }
