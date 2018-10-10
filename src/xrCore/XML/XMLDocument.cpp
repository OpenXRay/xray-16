#include "stdafx.h"
#pragma hdrstop

#include "XMLDocument.hpp"

XMLDocument::XMLDocument() : m_root(), m_pLocalRoot() {}

XMLDocument::~XMLDocument() { ClearInternal(); }

void XMLDocument::ClearInternal() { m_Doc.clear(); }

void ParseFile(pcstr path, CMemoryWriter& W, IReader* F, XMLDocument* xml)
{
    string4096 str;

    while (!F->eof())
    {
        F->r_string(str, sizeof str);

        if (str[0] && str[0] == '#' && strstr(str, "#include"))
        {
            string256 inc_name;
            if (_GetItem(str, 1, inc_name, '"'))
            {
#if defined(LINUX)
                while (char* sep = strchr(inc_name, '\\')) *sep = '/';
#endif
                IReader* I = nullptr;
                if (inc_name == strstr(inc_name, "ui" DELIMITER ))
                {
                    shared_str fn = xml->correct_file_name("ui", strchr(inc_name, _DELIMITER) + 1);
                    string_path buff;
                    strconcat(sizeof buff, buff, "ui" DELIMITER , fn.c_str());
                    I = FS.r_open(path, buff);
                }

                if (!I)
                    I = FS.r_open(path, inc_name);

                if (!I)
                    FATAL_F("XML file[%s] parsing failed. Can't find include file: [%s]", path, inc_name);
                ParseFile(path, W, I, xml);
                FS.r_close(I);
            }
        }
        else
            W.w_string(str);
    }
}

bool XMLDocument::Load(pcstr path_alias, pcstr path, pcstr xml_filename, bool fatal)
{
    shared_str fn = correct_file_name(path, xml_filename);

    string_path str;
    xr_sprintf(str, "%s" DELIMITER "%s", path, *fn);
    return Load(path_alias, str, fatal);
}

// Try to load from the first path, and if it's failed then try the second one
bool XMLDocument::Load(pcstr path_alias, pcstr path, pcstr path2, pcstr xml_filename, bool fatal /*= true*/)
{
    shared_str fn = correct_file_name(path, xml_filename);

    string_path str;
    xr_sprintf(str, "%s" DELIMITER "%s", path, *fn);
    if (Load(path_alias, str, false))
        return true;

    xr_sprintf(str, "%s" DELIMITER "%s", path2, *fn);
    return Load(path_alias, str, fatal);
}

// Load and parse xml file
bool XMLDocument::Load(pcstr path, pcstr xml_filename, bool fatal)
{
    IReader* F = FS.r_open(path, xml_filename);
    if (!F)
    {
        if (fatal)
            R_ASSERT3(F, "Can't find specified xml file", xml_filename);
        else
            return false;
    }

    xr_strcpy(m_xml_file_name, xml_filename);

    CMemoryWriter W;
    ParseFile(path, W, F, this);
    W.w_stringZ("");
    FS.r_close(F);

    return Set(reinterpret_cast<pcstr>(W.pointer()));
}

// XXX: support #include directive
bool XMLDocument::Set(pcstr text, bool fatal)
{
    R_ASSERT(text != nullptr);
    m_Doc.parse(text);

    if (m_Doc.isError())
    {
        string1024 str;
        xr_sprintf(str, "XML Error! File: %s Description: %s:%u \n", m_xml_file_name, m_Doc.error(), m_Doc.errorOffset());
        pcstr offsetted = text + m_Doc.errorOffset();

        if (fatal)
            R_ASSERT3(false, str, offsetted);
        else
            Log(str, offsetted);

        return false;
    }

    m_root = m_Doc.firstChildElement();

    return true;
}

XML_NODE XMLDocument::NavigateToNode(XML_NODE start_node, pcstr path, const size_t node_index) const
{
    R_ASSERT3(start_node && path, "NavigateToNode failed in XML file ", m_xml_file_name);
    XML_NODE node;
    string_path buf_str;
    VERIFY(xr_strlen(path) < 200);
    buf_str[0] = 0;
    xr_strcpy(buf_str, path);

    const char seps[] = ":";
    size_t tmp = 0;

    //разбить путь на отдельные подпути
    char* token = strtok(buf_str, seps);

    if (token != nullptr)
    {
        node = start_node.firstChild(token);

        while (tmp++ < node_index && node)
            node = node.nextSibling(token);
    }

    while (token)
    {
        // Get next token:
        token = strtok(nullptr, seps);

        if (token != nullptr)
            if (node)
            {
                const XML_NODE node_parent = node;
                node = node_parent.firstChild(token);
            }
    }

    return node;
}

XML_NODE XMLDocument::NavigateToNode(pcstr path, const size_t node_index) const
{
    return NavigateToNode(GetLocalRoot() ? GetLocalRoot() : GetRoot(), path, node_index);
}

XML_NODE XMLDocument::NavigateToNodeWithAttribute(pcstr tag_name, pcstr attrib_name, pcstr attrib_value)
{
    const XML_NODE root = GetLocalRoot() ? GetLocalRoot() : GetRoot();
    int tabsCount = GetNodesNum(root, tag_name);

    for (int i = 0; i < tabsCount; ++i)
    {
        pcstr result = ReadAttrib(root, tag_name, i, attrib_name, "");
        if (result && xr_strcmp(result, attrib_value) == 0)
        {
            return NavigateToNode(root, tag_name, i);
        }
    }
    return XML_NODE();
}

pcstr XMLDocument::Read(pcstr path, const size_t index, pcstr default_str_val) const
{
    const XML_NODE node = NavigateToNode(path, index);
    pcstr result = Read(node, default_str_val);
    return result;
}

pcstr XMLDocument::Read(XML_NODE start_node, pcstr path, const size_t index, pcstr default_str_val) const
{
    const XML_NODE node = NavigateToNode(start_node, path, index);
    pcstr result = Read(node, default_str_val);
    return result;
}

pcstr XMLDocument::Read(XML_NODE node, pcstr default_str_val) const
{
    if (!node)
        return default_str_val;

    node = node.firstChild();
    if (!node)
        return default_str_val;

    return node.textValueOr(default_str_val);
}

int XMLDocument::ReadInt(XML_NODE node, const int default_int_val) const
{
    pcstr result_str = Read(node, nullptr);

    if (result_str == nullptr)
        return default_int_val;

    return atoi(result_str);
}

int XMLDocument::ReadInt(pcstr path, const size_t index, const int default_int_val) const
{
    pcstr result_str = Read(path, index, nullptr);
    if (result_str == nullptr)
        return default_int_val;

    return atoi(result_str);
}

int XMLDocument::ReadInt(XML_NODE start_node, pcstr path, const size_t index, const int default_int_val) const
{
    pcstr result_str = Read(start_node, path, index, nullptr);
    if (result_str == nullptr)
        return default_int_val;

    return atoi(result_str);
}

float XMLDocument::ReadFlt(pcstr path, const size_t index, float default_flt_val) const
{
    pcstr result_str = Read(path, index, nullptr);
    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

float XMLDocument::ReadFlt(XML_NODE start_node, pcstr path, const size_t index, float default_flt_val) const
{
    pcstr result_str = Read(start_node, path, index, nullptr);
    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

float XMLDocument::ReadFlt(XML_NODE node, float default_flt_val) const
{
    pcstr result_str = Read(node, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

pcstr XMLDocument::ReadAttrib(XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, pcstr default_str_val) const
{
    const XML_NODE node = NavigateToNode(start_node, path, index);
    return ReadAttrib(node, attrib, default_str_val);
}

pcstr XMLDocument::ReadAttrib(pcstr path, const size_t index, pcstr attrib, pcstr default_str_val) const
{
    const XML_NODE node = NavigateToNode(path, index);
    return ReadAttrib(node, attrib, default_str_val);
}

pcstr XMLDocument::ReadAttrib(XML_NODE node, pcstr attrib, pcstr default_str_val) const
{
    if (!node)
        return default_str_val;

    pcstr result_str = node.elementAttribute(attrib);
    return result_str ? result_str : default_str_val;
}

int XMLDocument::ReadAttribInt(XML_NODE node, pcstr attrib, const int default_int_val) const
{
    pcstr result_str = ReadAttrib(node, attrib, nullptr);

    if (result_str == nullptr)
        return default_int_val;

    return atoi(result_str);
}

int XMLDocument::ReadAttribInt(pcstr path, const size_t index, pcstr attrib, int default_int_val) const
{
    pcstr result_str = ReadAttrib(path, index, attrib, nullptr);

    if (result_str == nullptr)
        return default_int_val;

    return atoi(result_str);
}

int XMLDocument::ReadAttribInt(XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, const int default_int_val) const
{
    pcstr result_str = ReadAttrib(start_node, path, index, attrib, nullptr);

    if (result_str == nullptr)
        return default_int_val;
    return atoi(result_str);
}

float XMLDocument::ReadAttribFlt(pcstr path, const size_t index, pcstr attrib, const float default_flt_val) const
{
    pcstr result_str = ReadAttrib(path, index, attrib, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

float XMLDocument::ReadAttribFlt(XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, const float default_flt_val) const
{
    pcstr result_str = ReadAttrib(start_node, path, index, attrib, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

float XMLDocument::ReadAttribFlt(XML_NODE node, pcstr attrib, const float default_flt_val) const
{
    pcstr result_str = ReadAttrib(node, attrib, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

size_t XMLDocument::GetNodesNum(pcstr path, const size_t index, pcstr tag_name) const
{
    XML_NODE node;
    const XML_NODE root = GetLocalRoot() ? GetLocalRoot() : GetRoot();

    if (path != nullptr)
    {
        node = NavigateToNode(path, index);

        if (!node)
            node = root;
    }
    else
        node = root;

    if (!node)
        return 0;

    return GetNodesNum(node, tag_name);
}

size_t XMLDocument::GetNodesNum(XML_NODE node, pcstr tag_name) const
{
    if (!node)
        return 0;

    XML_NODE el;

    if (!tag_name)
        el = node.firstChild();
    else
        el = node.firstChild(tag_name);

    size_t result = 0;

    while (el)
    {
        ++result;
        if (!tag_name)
            el = el.nextSibling();
        else
            el = el.nextSibling(tag_name);
    }

    return result;
}

//нахождение элемента по его атрибуту
XML_NODE XMLDocument::SearchForAttribute(
    pcstr path, const size_t index, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern) const
{
    const XML_NODE start_node = NavigateToNode(path, index);
    return SearchForAttribute(start_node, tag_name, attrib, attrib_value_pattern);;
}

XML_NODE XMLDocument::SearchForAttribute(
    XML_NODE start_node, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern) const
{
    while (start_node)
    {
        pcstr attribStr = start_node.elementAttribute(attrib);
        pcstr valueStr = start_node.elementValue();

        if (attribStr && 0 == xr_strcmp(attribStr, attrib_value_pattern) && valueStr &&
            0 == xr_strcmp(valueStr, tag_name))
            return start_node;

        XML_NODE newEl = start_node.firstChild(tag_name);
        newEl = SearchForAttribute(newEl, tag_name, attrib, attrib_value_pattern);
        if (newEl)
            return newEl;

        start_node = start_node.nextSibling(tag_name);
    }
    return XML_NODE();
}

#ifdef DEBUG // debug & mixed

pcstr XMLDocument::CheckUniqueAttrib(XML_NODE start_node, pcstr tag_name, pcstr attrib_name)
{
    m_AttribValues.clear();

    int tags_num = GetNodesNum(start_node, tag_name);

    for (int i = 0; i < tags_num; i++)
    {
        pcstr attrib = ReadAttrib(start_node, tag_name, i, attrib_name, nullptr);

        xr_vector<shared_str>::iterator it = std::find(m_AttribValues.begin(), m_AttribValues.end(), attrib);

        if (m_AttribValues.end() != it)
            return attrib;

        m_AttribValues.push_back(attrib);
    }
    return nullptr;
}
#endif
