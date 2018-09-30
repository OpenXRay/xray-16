// File:		KillMessageStruct.h
// Description:	storage for HUD message about player death
// Created:		10.03.2005
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

#include "xrUICore/ui_defs.h"

struct ColoredName
{
    shared_str m_name;
    u32 m_color;
};

struct IconInfo
{
    Frect m_rect;
    ui_shader m_shader;
};

struct KillMessageStruct
{
    ColoredName m_victim;
    IconInfo m_initiator;
    ColoredName m_killer;
    IconInfo m_ext_info;
};
