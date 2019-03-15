// file:		UITextureMaster.h
// description:	holds info about shared textures. able to initialize external controls
//				through IUITextureControl interface
// created:		11.05.2005
// author:		Serge Vynnychenko
// mail:		narrator@gsc-game.kiev.ua
//
// copyright 2005 GSC Game World

#include "pch.hpp"
#include "UITextureMaster.h"
#include "Static/UIStaticItem.h"
#include "uiabstract.h"
#include "xrUIXmlParser.h"
#include "Include/xrRender/UIShader.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"
#include <iostream>

xr_map<shared_str, TEX_INFO> CUITextureMaster::m_textures;
xr_map<sh_pair, ui_shader> CUITextureMaster::m_shaders;

void CUITextureMaster::FreeTexInfo()
{
    m_textures.clear();
    FreeCachedShaders();
}

void CUITextureMaster::FreeCachedShaders() { m_shaders.clear(); }
void CUITextureMaster::ParseShTexInfo(pcstr path, pcstr xml_file)
{
    CUIXml xml;
    xml.Load(CONFIG_PATH, path, xml_file);

    const int files_num = xml.GetNodesNum("", 0, "file");

    for (int fi = 0; fi < files_num; ++fi)
    {
        XML_NODE root_node = xml.GetLocalRoot();
        shared_str file = xml.ReadAttrib("file", fi, "name");

        XML_NODE node = xml.NavigateToNode("file", fi);

        const int num = xml.GetNodesNum(node, "texture");
        for (int i = 0; i < num; i++)
        {
            TEX_INFO info;

            info.file = file;

            info.rect.x1 = xml.ReadAttribFlt(node, "texture", i, "x");
            info.rect.x2 = xml.ReadAttribFlt(node, "texture", i, "width") + info.rect.x1;
            info.rect.y1 = xml.ReadAttribFlt(node, "texture", i, "y");
            info.rect.y2 = xml.ReadAttribFlt(node, "texture", i, "height") + info.rect.y1;
            shared_str id = xml.ReadAttrib(node, "texture", i, "id");
            /* avo: fix issue when values were not updated (silently skipped) when same key is encountered more than once. This is how std::map is designed. 
            /* Also used more efficient C++11 std::map::emplace method instead of outdated std::pair::make_pair */
            /* XXX: avo: note that xxx.insert(mk_pair(v1,v2)) pattern is used extensively throughout solution so there is a good potential for other bug fixes/improvements */
            static Lock new_texture_lock;
            ScopeLock new_texture_guard(&new_texture_lock);
            if (m_textures.find(id) == m_textures.end())
                m_textures.emplace(id, info);
            else
                m_textures[id] = info;
            /* avo: end */
            //m_textures.insert(mk_pair(id,info)); // original GSC insert call
        }

        xml.SetLocalRoot(root_node);
    }
}

void CUITextureMaster::ParseShTexInfo(pcstr xml_file)
{
    CUIXml xml;
    xml.Load(CONFIG_PATH, UI_PATH, xml_file);
    const shared_str file = xml.Read("file_name", 0, "");

    const int num = xml.GetNodesNum("", 0, "texture");
    for (int i = 0; i < num; i++)
    {
        TEX_INFO info;

        info.file = file;

        info.rect.x1 = xml.ReadAttribFlt("texture", i, "x");
        info.rect.x2 = xml.ReadAttribFlt("texture", i, "width") + info.rect.x1;
        info.rect.y1 = xml.ReadAttribFlt("texture", i, "y");
        info.rect.y2 = xml.ReadAttribFlt("texture", i, "height") + info.rect.y1;
        shared_str id = xml.ReadAttrib("texture", i, "id");

        if (m_textures.find(id) == m_textures.end())
            m_textures.emplace(id, info);
        else
            m_textures[id] = info;
    }
}

bool CUITextureMaster::IsSh(const shared_str& texture_name)
{
    return strstr(texture_name.c_str(), DELIMITER) ? false : true;
}

void CUITextureMaster::InitTexture(
    const shared_str& texture_name, const shared_str& shader_name, ui_shader& out_shader, Frect& out_rect)
{
    xr_map<shared_str, TEX_INFO>::iterator it = m_textures.find(texture_name);
    if (it != m_textures.end())
    {
        sh_pair p = {it->second.file, shader_name};
        xr_map<sh_pair, ui_shader>::iterator sh_it = m_shaders.find(p);
        if (sh_it == m_shaders.end())
            m_shaders[p]->create(shader_name.c_str(), it->second.file.c_str());

        out_shader = m_shaders[p];
        out_rect = (*it).second.rect;
    }
    else
        out_shader->create(shader_name.c_str(), texture_name.c_str());
}

void CUITextureMaster::InitTexture(const shared_str& texture_name, CUIStaticItem* tc, const shared_str& shader_name)
{
    xr_map<shared_str, TEX_INFO>::iterator it = m_textures.find(texture_name);
    if (it != m_textures.end())
    {
        sh_pair p = {it->second.file, shader_name};
        xr_map<sh_pair, ui_shader>::iterator sh_it = m_shaders.find(p);
        if (sh_it == m_shaders.end())
            m_shaders[p]->create(shader_name.c_str(), it->second.file.c_str());

        tc->SetShader(m_shaders[p]);
        tc->SetTextureRect((*it).second.rect);
        tc->SetSize(Fvector2().set(it->second.rect.width(), it->second.rect.height()));
    }
    else
        tc->CreateShader(texture_name.c_str(), shader_name.c_str());
}

Frect CUITextureMaster::GetTextureRect(const shared_str& texture_name)
{
    TEX_INFO info = FindItem(texture_name);
    return info.rect;
}

pcstr CUITextureMaster::GetTextureFileName(pcstr texture_name)
{
    TEX_INFO info = FindItem(texture_name);
    return info.file.c_str();
}

float CUITextureMaster::GetTextureHeight(const shared_str& texture_name)
{
    TEX_INFO info = FindItem(texture_name);
    return info.rect.height();
}

float CUITextureMaster::GetTextureWidth(const shared_str& texture_name)
{
    TEX_INFO info = FindItem(texture_name);
    return info.rect.width();
}

TEX_INFO CUITextureMaster::FindItem(const shared_str& texture_name, pcstr default_texture /*= nullptr*/)
{
    auto it = m_textures.find(texture_name);

    if (it != m_textures.end())
        return (it->second);

    it = m_textures.find(default_texture);
    if (it != m_textures.end())
        return (it->second);

    VERIFY4(false, "Can't find texture", texture_name.c_str(), default_texture);
    return TEX_INFO();
}

void CUITextureMaster::GetTextureShader(const shared_str& texture_name, ui_shader& sh)
{
    xr_map<shared_str, TEX_INFO>::iterator it;
    it = m_textures.find(texture_name);

    R_ASSERT3(it != m_textures.end(), "can't find texture", texture_name.c_str());

    sh->create("hud" DELIMITER "default", *((*it).second.file));
}
