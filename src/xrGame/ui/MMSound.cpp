#include "StdAfx.h"

#include "MMSound.h"
#include "xrUICore/XML/xrUIXmlParser.h"

CMMSound::CMMSound() {}
CMMSound::~CMMSound() { all_Stop(); }
void CMMSound::Init(CUIXml& xml_doc, LPCSTR path)
{
    string256 _path;
    m_bRandom = xml_doc.ReadAttribInt(path, 0, "random") ? true : false;

    int nodes_num = xml_doc.GetNodesNum(path, 0, "menu_music");

    XML_NODE tab_node = xml_doc.NavigateToNode(path, 0);
    xml_doc.SetLocalRoot(tab_node);
    for (int i = 0; i < nodes_num; ++i)
        m_play_list.push_back(xml_doc.Read("menu_music", i, ""));
    xml_doc.SetLocalRoot(xml_doc.GetRoot());

    strconcat(sizeof(_path), _path, path, ":whell_sound");
    if (check_file(xml_doc.Read(_path, 0, "")))
        m_whell.create(xml_doc.Read(_path, 0, ""), st_Effect, sg_SourceType);

    strconcat(sizeof(_path), _path, path, ":whell_click");
    if (check_file(xml_doc.Read(_path, 0, "")))
        m_whell_click.create(xml_doc.Read(_path, 0, ""), st_Effect, sg_SourceType);
}

bool CMMSound::check_file(LPCSTR fname)
{
    string_path _path;
    strconcat(sizeof(_path), _path, fname, ".ogg");
    return FS.exist("$game_sounds$", _path) ? true : false;
}

void CMMSound::whell_Play()
{
    if (m_whell._handle() && !m_whell._feedback())
        m_whell.play(NULL, sm_Looped | sm_2D);
}

void CMMSound::whell_Stop()
{
    if (m_whell._feedback())
        m_whell.stop();
}

void CMMSound::whell_Click()
{
    if (m_whell_click._handle())
        m_whell_click.play(NULL, sm_2D);
}

void CMMSound::whell_UpdateMoving(float frequency) { m_whell.set_frequency(frequency); }
void CMMSound::music_Play()
{
    if (m_play_list.empty())
        return;

    const int i = Random.randI(m_play_list.size());

    string_path _stereo, _l, _r;
    strconcat(sizeof(_stereo), _stereo, m_play_list[i].c_str(), ".ogg");
    strconcat(sizeof(_l), _l, m_play_list[i].c_str(), "_l.ogg");
    strconcat(sizeof(_r), _r, m_play_list[i].c_str(), "_r.ogg");

    bool found[channels_count + 1];
    ref_sound separated[channels_count];
    found[0] = separated[0].create(_l, st_Effect, sg_Undefined, false);
    found[1] = separated[1].create(_r, st_Effect, sg_Undefined, false);

    ref_sound one;
    found[channels_count] = one.create(_stereo, st_Music, sg_SourceType, !(found[0] && found[1]));

    if (!found[channels_count] && found[0] && found[1])
    {
        m_music[0] = separated[0];
        m_music[1] = separated[1];
        m_music[0].play_at_pos(nullptr, Fvector().set(-0.5f, 0.f, 0.3f), sm_2D);
        m_music[1].play_at_pos(nullptr, Fvector().set(+0.5f, 0.f, 0.3f), sm_2D);
    }
    else
    {
        m_music[0] = one;
        m_music[0].play(nullptr, sm_2D);
        m_music[1].destroy();
    }
}

void CMMSound::music_Update()
{
    if (Device.Paused())
        return;

    if (!m_music[0]._feedback() || (m_music[1]._handle() && !m_music[1]._feedback()))
        music_Play();
}

void CMMSound::music_Stop()
{
    for (ref_sound& channel : m_music)
        channel.stop();
}
void CMMSound::all_Stop()
{
    music_Stop();
    m_whell.stop();
    m_whell_click.stop();
}
