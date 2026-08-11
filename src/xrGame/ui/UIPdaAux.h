//=============================================================================
//  Filename:   UIPdaAux.h
//	Created by Roman E. Marchenko, vortex@gsc-game.kiev.ua
//	Copyright 2004. GSC Game World
//	---------------------------------------------------------------------------
//  Некоторые определения которые общие для всех диалогов ПДА
//=============================================================================

#pragma once

enum EPdaTabs
{
    eptQuests = 0,
    eptMap,
    eptDiary,
    eptContacts,
    eptRanking,
    eptActorStatistic,
    eptEncyclopedia,
    eptNoActiveTab = u16(-1)
};

extern const char* const ALL_PDA_HEADER_PREFIX;

namespace pda_section
{
enum part
{
    quests = (1 << 8),
    map = (1 << 9),
    diary = (1 << 10),
    contacts = (1 << 11),
    ranking = (1 << 12),
    statistics = (1 << 13),
    encyclopedia = (1 << 14),

    news = diary | (1 << 1),
    info = diary | (1 << 2),
    journal = diary | (1 << 3),

};
};