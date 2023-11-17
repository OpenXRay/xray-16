#include "stdafx.h"

#include "XMLDocument.hpp"

pcstr UI_PATH = UI_PATH_DEFAULT;
pcstr UI_PATH_WITH_DELIMITER = UI_PATH_DEFAULT_WITH_DELIMITER;

XMLDocument::XMLDocument() : m_xml_file_name(), m_root(nullptr), m_pLocalRoot(nullptr), m_bIgnoreMissingEndTagError(false) {}

XMLDocument::~XMLDocument() { ClearInternal(); }

void XMLDocument::ClearInternal() { m_Doc.Clear(); }

enum class ParseIncludeResult
{
    Success,   /// There is a valid #include and 'out_include_name' returns the filename
    Error,     /// There is a #include but there is some problem
    NoInclude, /// There is no #include on this line
};

// Given a string of the form: '#include "filename"' we return the filename in 'out_include_name'
ParseIncludeResult ParseInclude(pstr string, pcstr& out_include_name)
{
    // Skip any whitespace characters
    while (*string != '\0' && std::isblank(*string))
    {
        ++string;
    }

    // Check for #include
    static constexpr pcstr IncludeTag = "#include";
    if (std::strncmp(string, IncludeTag, 8) != 0)
        return ParseIncludeResult::NoInclude;

    string += 8;

    // Skip any whitespace characters
    while (*string != '\0' && std::isblank(*string))
        ++string;

    // Check that after the tag there is a quote
    if (*string != '\"')
        return ParseIncludeResult::Error;

    // Mark the start of the include name
    ++string;
    out_include_name = string;

    while (*string != '\0' && *string != '\"')
        ++string;

    // Check for unterminated or empty include name
    if (*string == '\0' || out_include_name == string)
        return ParseIncludeResult::Error;

    // Check for unreasonably long include names
    const size_t size = string - out_include_name;
    if (size > 1024)
        return ParseIncludeResult::Error;

    // NOTE(Andre): Yes this might look scary but it's perfectly fine. Since the include name is already in the string
    // we are parsing and its not used afterwards we simply replace the closing quote with a null byte and we have a
    // valid c-string pointed to by 'out_include_name' and safe ourselves the need to copy the string.
    *string = '\0';

    return ParseIncludeResult::Success;
}

void ParseFile(pcstr path, CMemoryWriter& W, IReader* F, XMLDocument* xml, bool fatal, u8 include_depth)
{
    // Prevent stack overflow due to recursive or cyclic includes
    if (include_depth >= 128)
    {
        R_ASSERT3(!fatal, "XML file[%s] parsing failed. Maximum include depth reached (> 128)", path);
        Msg("! XML file[%s] parsing failed. Maximum include depth reached (> 128)", path);
        return;
    }

    const auto tryOpenFile = [&](IReader*& file, pcstr includeName, pcstr comparePath, pcstr uiPath, pcstr uiPathDelim)
    {
        if (file)
            return;
        if (includeName == strstr(includeName, comparePath))
        {
            pcstr fileName = strstr(includeName, comparePath);
            fileName = fileName ? ++fileName : includeName;

            shared_str fn = xml->correct_file_name(uiPath, fileName);
            string_path buff;
            strconcat(buff, uiPathDelim, fn.c_str());
            file = FS.r_open(path, buff);
        }
    };

    while (!F->eof())
    {
        string4096 str;
        if (!F->try_r_string(str, sizeof(str)))
        {
            R_ASSERT3(!fatal, "XML file[%s] parsing failed. Line is too long (>= 4096)", path);
            Msg("! XML file[%s] parsing failed. Line is too long (>= 4096)", path);
            return;
        }

        pcstr inc_name;
        switch (ParseInclude(str, inc_name))
        {
            case ParseIncludeResult::Success:
            {
                IReader* I = nullptr;
                tryOpenFile(I, inc_name, UI_PATH, UI_PATH, UI_PATH_WITH_DELIMITER);
                tryOpenFile(I, inc_name, UI_PATH_DEFAULT_WITH_DELIMITER, UI_PATH, UI_PATH_WITH_DELIMITER);
                tryOpenFile(I, inc_name, UI_PATH_DEFAULT_WITH_DELIMITER, UI_PATH_DEFAULT, UI_PATH_DEFAULT_WITH_DELIMITER);

                if (!I)
                    I = FS.r_open(path, inc_name);

                if (!I)
                {
                    R_ASSERT4(!fatal, "XML file[%s] parsing failed. Can't find include file: [%s]", path, inc_name);
                    Msg("! XML file[%s] parsing failed. Can't find include file: [%s]", path, inc_name);
                    return;
                }

                ParseFile(path, W, I, xml, fatal, include_depth + 1);
                FS.r_close(I);
                break;
            }

            case ParseIncludeResult::Error:
                R_ASSERT4(!fatal, "XML file[%s] invalid include directive: '%s'", path, str);
                Msg("! XML file[%s] invalid include directive: '%s'", path, str);
                break;

            case ParseIncludeResult::NoInclude:
                W.w_string(str);
                break;
        }
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

    fn = correct_file_name(path2, xml_filename);
    xr_sprintf(str, "%s" DELIMITER "%s", path2, *fn);
    return Load(path_alias, str, fatal);
}

// Load and parse xml file
bool XMLDocument::Load(pcstr path, pcstr xml_filename, bool fatal)
{
    IReader* F = FS.r_open(path, xml_filename);
    if (!F)
    {
        R_ASSERT3(!fatal, "Can't find specified xml file", xml_filename);
        return false;
    }

    xr_strcpy(m_xml_file_name, xml_filename);

    CMemoryWriter W;
    ParseFile(path, W, F, this, fatal, 0);
    W.w_stringZ("");
    FS.r_close(F);

    return Set(reinterpret_cast<pcstr>(W.pointer()), fatal);
}

bool XMLDocument::Set(pcstr text, bool fatal)
{
    R_ASSERT(text != nullptr);
    m_Doc.Parse(&m_Doc, text);

    if (m_Doc.Error())
    {
        const bool canSkipError = IgnoringMissingEndTagError() && m_Doc.ErrorId() == TiXmlBase::TIXML_ERROR_READING_END_TAG;
        R_ASSERT3(!fatal || canSkipError, m_Doc.ErrorDesc(), m_xml_file_name);
        if (!canSkipError)
            return false;
    }

    m_root = m_Doc.FirstChildElement();

    return true;
}

XML_NODE XMLDocument::NavigateToNode(CONST_XML_NODE start_node, pcstr path, const size_t node_index) const
{
    R_ASSERT3(start_node && path, "NavigateToNode failed in XML file ", m_xml_file_name);
    CONST_XML_NODE node{};
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
        node = start_node->FirstChild(token);

        while (tmp++ < node_index && node)
            node = node->NextSibling(token);
    }

    while (token)
    {
        // Get next token:
        token = strtok(nullptr, seps);

        if (token != nullptr)
            if (node)
            {
                CONST_XML_NODE node_parent = node;
                node = node_parent->FirstChild(token);
            }
    }

    return const_cast<XML_NODE>(node);
}

XML_NODE XMLDocument::NavigateToNode(pcstr path, const size_t node_index) const
{
    return NavigateToNode(GetLocalRoot() ? GetLocalRoot() : GetRoot(), path, node_index);
}

XML_NODE XMLDocument::NavigateToNodeWithAttribute(pcstr tag_name, pcstr attrib_name, pcstr attrib_value) const
{
    CONST_XML_NODE root = GetLocalRoot() ? GetLocalRoot() : GetRoot();
    int tabsCount = GetNodesNum(root, tag_name);

    for (int i = 0; i < tabsCount; ++i)
    {
        pcstr result = ReadAttrib(root, tag_name, i, attrib_name, "");
        if (result && xr_strcmp(result, attrib_value) == 0)
        {
            return NavigateToNode(root, tag_name, i);
        }
    }
    return nullptr;
}

pcstr XMLDocument::Read(pcstr path, const size_t index, pcstr default_str_val) const
{
    CONST_XML_NODE node = NavigateToNode(path, index);
    pcstr result = Read(node, default_str_val);
    return result;
}

pcstr XMLDocument::Read(CONST_XML_NODE start_node, pcstr path, const size_t index, pcstr default_str_val) const
{
    CONST_XML_NODE node = NavigateToNode(start_node, path, index);
    pcstr result = Read(node, default_str_val);
    return result;
}

pcstr XMLDocument::Read(CONST_XML_NODE node, pcstr default_str_val) const
{
    if (!node)
        return default_str_val;

    node = node->FirstChild();
    if (!node)
        return default_str_val;

    const auto text = node->ToText();
    if (text)
        return text->Value();

    return default_str_val;
}

int XMLDocument::ReadInt(CONST_XML_NODE node, const int default_int_val) const
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

int XMLDocument::ReadInt(CONST_XML_NODE start_node, pcstr path, const size_t index, const int default_int_val) const
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

float XMLDocument::ReadFlt(CONST_XML_NODE start_node, pcstr path, const size_t index, float default_flt_val) const
{
    pcstr result_str = Read(start_node, path, index, nullptr);
    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

float XMLDocument::ReadFlt(CONST_XML_NODE node, float default_flt_val) const
{
    pcstr result_str = Read(node, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

pcstr XMLDocument::ReadAttrib(CONST_XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, pcstr default_str_val) const
{
    CONST_XML_NODE node = NavigateToNode(start_node, path, index);
    return ReadAttrib(node, attrib, default_str_val);
}

pcstr XMLDocument::ReadAttrib(pcstr path, const size_t index, pcstr attrib, pcstr default_str_val) const
{
    CONST_XML_NODE node = NavigateToNode(path, index);
    return ReadAttrib(node, attrib, default_str_val);
}

pcstr XMLDocument::ReadAttrib(CONST_XML_NODE node, pcstr attrib, pcstr default_str_val) const
{
    pcstr result = nullptr;
    if (node)
    {
        // Кастаем ниже по иерархии
        const auto el = node->ToElement();

        if (el)
            result = el->Attribute(attrib);
    }

    return result ? result : default_str_val;
}

int XMLDocument::ReadAttribInt(CONST_XML_NODE node, pcstr attrib, const int default_int_val) const
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

int XMLDocument::ReadAttribInt(CONST_XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, const int default_int_val) const
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

float XMLDocument::ReadAttribFlt(CONST_XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, const float default_flt_val) const
{
    pcstr result_str = ReadAttrib(start_node, path, index, attrib, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

float XMLDocument::ReadAttribFlt(CONST_XML_NODE node, pcstr attrib, const float default_flt_val) const
{
    pcstr result_str = ReadAttrib(node, attrib, nullptr);

    if (result_str == nullptr)
        return default_flt_val;

    return static_cast<float>(atof(result_str));
}

size_t XMLDocument::GetNodesNum(pcstr path, const size_t index, pcstr tag_name) const
{
    CONST_XML_NODE node;
    CONST_XML_NODE root = GetLocalRoot() ? GetLocalRoot() : GetRoot();

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

size_t XMLDocument::GetNodesNum(CONST_XML_NODE node, pcstr tag_name, bool includingComments /*= true*/) const
{
    if (!node)
        return 0;

    CONST_XML_NODE el;

    if (!tag_name)
        el = node->FirstChild();
    else
        el = node->FirstChild(tag_name);

    size_t result = 0;

    while (el)
    {
        if (includingComments || el->Type() != TiXmlNode::NodeType::COMMENT)
            ++result;
        if (!tag_name)
            el = el->NextSibling();
        else
            el = el->NextSibling(tag_name);
    }

    return result;
}

//нахождение элемента по его атрибуту
XML_NODE XMLDocument::SearchForAttribute(
    pcstr path, const size_t index, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern) const
{
    CONST_XML_NODE start_node = NavigateToNode(path, index);
    return SearchForAttribute(start_node, tag_name, attrib, attrib_value_pattern);;
}

XML_NODE XMLDocument::SearchForAttribute(
    CONST_XML_NODE start_node, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern) const
{
    while (start_node)
    {
        const auto el = start_node->ToElement();
        if (el)
        {
            pcstr attribStr = el->Attribute(attrib);
            pcstr valueStr = el->Value();


            if (attribStr && 0 == xr_strcmp(attribStr, attrib_value_pattern) && valueStr &&
                0 == xr_strcmp(valueStr, tag_name))
            {
                return const_cast<TiXmlElement*>(el);
            }
        }

        CONST_XML_NODE newEl = start_node->FirstChild(tag_name);
        newEl = SearchForAttribute(newEl, tag_name, attrib, attrib_value_pattern);
        if (newEl)
            return const_cast<XML_NODE>(newEl);

        start_node = start_node->NextSibling(tag_name);
    }

    return nullptr;
}

pcstr XMLDocument::CheckUniqueAttrib(CONST_XML_NODE start_node, pcstr tag_name, pcstr attrib_name)
{
    m_AttribValues.clear();

    int tags_num = GetNodesNum(start_node, tag_name);

    for (int i = 0; i < tags_num; i++)
    {
        pcstr attrib = ReadAttrib(start_node, tag_name, i, attrib_name, nullptr);

        auto it = std::find(m_AttribValues.begin(), m_AttribValues.end(), attrib);

        if (m_AttribValues.end() != it)
            return attrib;

        m_AttribValues.push_back(attrib);
    }

    m_AttribValues.clear();
    return nullptr;
}
