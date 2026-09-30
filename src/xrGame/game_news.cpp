///////////////////////////////////////////////////////////////
// game_news.cpp
// реестр новостей: новости симуляции + сюжетные
///////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "game_news.h"
#include "Common/object_broker.h"

// #include "date_time.h"

// XXX: introduce PROPER versioning for saves instead of such hacks
constexpr char TEXTURE_RECT_MARKER[] = "$TEXTURE_RECT$";

void GAME_NEWS_DATA::save(IWriter& stream)
{
    save_data(m_type, stream);
    save_data(news_caption, stream);
    save_data(news_text, stream);
    save_data(receive_time, stream);
    save_data(texture_name, stream);
    stream.w_stringZ(TEXTURE_RECT_MARKER);
    save_data(tex_rect, stream);
}

void GAME_NEWS_DATA::load(IReader& stream)
{
    load_data(m_type, stream);
    load_data(news_caption, stream);
    load_data(news_text, stream);
    load_data(receive_time, stream);
    load_data(texture_name, stream);

    const char* data = static_cast<char*>(stream.pointer());
    constexpr auto marker_size = sizeof(TEXTURE_RECT_MARKER);

    if (stream.elapsed() < marker_size || memcmp(data, TEXTURE_RECT_MARKER, marker_size) != 0)
    {
        tex_rect = {};
        return;
    }

    stream.advance(marker_size);
    load_data(tex_rect, stream);
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
