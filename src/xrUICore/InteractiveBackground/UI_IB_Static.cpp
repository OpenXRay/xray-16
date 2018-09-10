// File:		UI_IB_Static.h
// Description:	Inheritance of UIInteractiveBackground template class with some
//				CUIStatic features
// Created:		09.02.2005
// Author:		Serge Vynnychenko
// Mail:		narrator@gsc-game.kiev.ua

// Copyright 2005 GSC Game World

#include "pch.hpp"
#include "UI_IB_Static.h"

void CUI_IB_Static::SetTextureOffset(float x, float y)
{
    for (int i = 0; i < S_Total; ++i)
        if (m_states[i])
            m_states[i]->SetTextureOffset(x, y);
}

void CUI_IB_Static::SetStretchTexture(bool stretch_texture)
{
    for (int i = 0; i < S_Total; ++i)
        if (m_states[i])
            m_states[i]->SetStretchTexture(stretch_texture);
}
