///////////////////////////////////////////////////////////////
// game_news.cpp
// реестр новостей: новости симуляции + сюжетные
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "game_news.h"
#include "Common/object_broker.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "xrAICore/Navigation/game_graph.h"

#include "date_time.h"
#include "xrServer_Objects_ALife_Monsters.h"

#include "specific_character.h"

GAME_NEWS_DATA::GAME_NEWS_DATA() : receive_time(0)
{
    m_type = eNews;
    //	tex_rect.set	(0.0f,0.0f,0.0f,0.0f);
    show_time = DEFAULT_NEWS_SHOW_TIME;
}

void GAME_NEWS_DATA::save(IWriter& stream)
{
    save_data(m_type, stream);
    save_data(news_caption, stream);
    save_data(news_text, stream);
    save_data(receive_time, stream);
    save_data(texture_name, stream);
    //	save_data(tex_rect,		stream);
}

void GAME_NEWS_DATA::load(IReader& stream)
{
    load_data(m_type, stream);
    load_data(news_caption, stream);
    load_data(news_text, stream);
    load_data(receive_time, stream);
    load_data(texture_name, stream);
    //	load_data(tex_rect,		stream);
}
/*
LPCSTR GAME_NEWS_DATA::SingleLineText()
{
    if( xr_strlen(full_news_text.c_str()) )
        return full_news_text.c_str();
    string128	time = "";

    // Calc current time
    u32 years, months, days, hours, minutes, seconds, milliseconds;
    split_time		(receive_time, years, months, days, hours, minutes, seconds, milliseconds);
//#pragma todo("Satan->Satan : insert carry-over")
    //xr_sprintf(time, "%02i:%02i \\n", hours, minutes);
    xr_sprintf		(time, "%02i:%02i, ", hours, minutes);
//	strconcat	(result, locationName, time, newsPhrase);

    full_news_text			= time;
//	full_news_text			+= "%c[255,189,189,224] ";
    full_news_text			+= news_caption.c_str();
    full_news_text			+= " ";
//	full_news_text			+= " %c[default]";
    full_news_text			+= news_text.c_str();

    return full_news_text.c_str();
}
*/
