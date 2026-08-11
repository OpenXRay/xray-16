#pragma once

#include "alife_space.h"
#include "Common/object_interfaces.h"
#include "xrCore/_fbox2.h"

#define DEFAULT_NEWS_SHOW_TIME 5000

struct GAME_NEWS_DATA : public ISerializable
{
    enum eNewsType
    {
        eNews = 0,
        eTalk = 1
    } m_type{};

    virtual void load(IReader&);
    virtual void save(IWriter&);

    int show_time{ DEFAULT_NEWS_SHOW_TIME };
    shared_str news_caption;
    shared_str news_text;
    shared_str texture_name;
    // Keep this data transient to preserve the existing save-file format.
    Frect texture_rect{};
    bool has_texture_rect{};
    ALife::_TIME_ID receive_time;
};

using GAME_NEWS_VECTOR = xr_vector<GAME_NEWS_DATA>;
