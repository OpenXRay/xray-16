#pragma once
#ifndef xrXMLParserH
#define xrXMLParserH

#include "tinyxml.h"

#include "xrCommon/xr_vector.h"
#include "xrCore/xrstring.h"

// XXX: interesting idea is to have variable configs folder. Need we?
static constexpr pcstr CONFIG_PATH = _game_config_;
static constexpr pcstr UI_PATH_DEFAULT = "ui";
static constexpr pcstr UI_PATH_DEFAULT_WITH_DELIMITER = "ui" DELIMITER;
XRCORE_API extern pcstr UI_PATH;
XRCORE_API extern pcstr UI_PATH_WITH_DELIMITER;

using XML_NODE = TiXmlNode*;
using XML_DOC  = TiXmlDocument;

class XRCORE_API XMLDocument : public Noncopyable
{

public:
    string_path m_xml_file_name;
    XMLDocument();
    virtual ~XMLDocument();
    void ClearInternal();

    bool Load(pcstr path_alias, pcstr xml_filename, bool fatal = true);
    bool Load(pcstr path_alias, pcstr path, pcstr xml_filename, bool fatal = true);
    bool Load(pcstr path_alias, pcstr path, pcstr path2, pcstr xml_filename, bool fatal = true);

    // Set XML directly. Doesn't support #include directive
    bool Set(pcstr text, bool fatal = true);

    //чтение элементов
    pcstr Read(pcstr path, const size_t index, pcstr default_str_val) const;
    pcstr Read(XML_NODE start_node, pcstr path, const size_t index, pcstr default_str_val) const;
    pcstr Read(XML_NODE node, pcstr default_str_val) const;

    int ReadInt(pcstr path, const size_t index, const int default_int_val) const;
    int ReadInt(XML_NODE start_node, pcstr path, const size_t index, const int default_int_val) const;
    int ReadInt(XML_NODE node, const int default_int_val) const;

    float ReadFlt(pcstr path, const size_t index, float default_flt_val) const;
    float ReadFlt(XML_NODE start_node, pcstr path, const size_t index, float default_flt_val) const;
    float ReadFlt(XML_NODE node, float default_flt_val) const;

    pcstr ReadAttrib(pcstr path, const size_t index, pcstr attrib, pcstr default_str_val = "") const;
    pcstr ReadAttrib(XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, pcstr default_str_val = "") const;
    pcstr ReadAttrib(XML_NODE node, pcstr attrib, pcstr default_str_val) const;

    int ReadAttribInt(pcstr path, const size_t index, pcstr attrib, int default_int_val = 0) const;
    int ReadAttribInt(XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, const int default_int_val = 0) const;
    int ReadAttribInt(XML_NODE node, pcstr attrib, const int default_int_val) const;

    float ReadAttribFlt(pcstr path, const size_t index, pcstr attrib, const float default_flt_val = 0.0f) const;
    float ReadAttribFlt(XML_NODE start_node, pcstr path, const size_t index, pcstr attrib, const float default_flt_val = 0.0f) const;
    float ReadAttribFlt(XML_NODE node, pcstr attrib, const float default_flt_val = 0.0f) const;

    XML_NODE SearchForAttribute(pcstr path, const size_t index, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern) const;
    XML_NODE SearchForAttribute(XML_NODE start_node, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern) const;

    //возвращает количество узлов с заданым именем
    size_t GetNodesNum(pcstr path, const size_t index, pcstr tag_name) const;
    size_t GetNodesNum(XML_NODE node, pcstr tag_name) const;

    //проверка того, что аттрибуты у тегов уникальны
    //(если не NULL, то уникальность нарушена и возврашается имя
    //повторяющегося атрибута)
    pcstr CheckUniqueAttrib(XML_NODE start_node, pcstr tag_name, pcstr attrib_name);

    //переместиться по XML дереву
    //путь задается в форме PARENT:CHILD:CHIDLS_CHILD
    // node_index - номер, если узлов с одним именем несколько
    XML_NODE NavigateToNode(pcstr path, const size_t node_index = 0) const;
    XML_NODE NavigateToNode(XML_NODE start_node, pcstr path, const size_t node_index = 0) const;
    XML_NODE NavigateToNodeWithAttribute(pcstr tag_name, pcstr attrib_name, pcstr attrib_value);

    void SetLocalRoot(XML_NODE pLocalRoot) { m_pLocalRoot = pLocalRoot; }
    XML_NODE GetLocalRoot() const { return m_pLocalRoot; }
    XML_NODE GetRoot() const { return m_root; }

protected:
    XML_NODE m_root;
    XML_NODE m_pLocalRoot;

    //буфферный вектор для проверки уникальность аттрибутов
    xr_vector<shared_str> m_AttribValues;

public:
    virtual shared_str correct_file_name(pcstr path, pcstr fn) { return fn; }

private:
    XML_DOC m_Doc;
};

#endif // xrXMLParserH
