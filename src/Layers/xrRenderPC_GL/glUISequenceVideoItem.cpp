#include "stdafx.h"
#include "glUISequenceVideoItem.h"

glUISequenceVideoItem::glUISequenceVideoItem()
{
	m_texture = 0;
}

void glUISequenceVideoItem::Copy(IUISequenceVideoItem&_in)
{
	*this = *((glUISequenceVideoItem*)&_in);
}

void glUISequenceVideoItem::CaptureTexture()
{
	m_texture = RCache.get_ActiveTexture(0);
}
