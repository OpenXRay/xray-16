#include "stdafx.h"
#include "dxUISequenceVideoItem.h"

dxUISequenceVideoItem::dxUISequenceVideoItem() { m_texture = nullptr; }
void dxUISequenceVideoItem::Copy(IUISequenceVideoItem& _in) { *this = *((dxUISequenceVideoItem*)&_in); }

void dxUISequenceVideoItem::CaptureTexture()
{
    R_constant* C = RCache.get_c(c_sbase)._get(); // get sampler
    m_texture = RCache.get_ActiveTexture(C ? C->samp.index : 0);
    R_ASSERT(m_texture);
}
