#ifndef xrXMLParserH
#define xrXMLParserH
#pragma once

#include "Common/Platform.hpp"

const pcstr CONFIG_PATH = "$game_config$";
const pcstr UI_PATH = "ui";

#include "tinyxml.h"

typedef TiXmlNode XML_NODE;
typedef TiXmlAttribute XML_ATTRIBUTE;

class XRCORE_API XMLDocument
{
    void Load(pcstr path_alias, pcstr xml_filename);

public:
    string_path m_xml_file_name;
    XMLDocument();
    virtual ~XMLDocument();
    void ClearInternal();

    void Load(pcstr path_alias, pcstr path, pcstr xml_filename);

    //чтение элементов
    pcstr Read(pcstr path, int index, pcstr default_str_val);
    pcstr Read(XML_NODE* start_node, pcstr path, int index, pcstr default_str_val);
    pcstr Read(XML_NODE* node, pcstr default_str_val);

    int ReadInt(pcstr path, int index, int default_int_val);
    int ReadInt(XML_NODE* start_node, pcstr path, int index, int default_int_val);
    int ReadInt(XML_NODE* node, int default_int_val);

    float ReadFlt(pcstr path, int index, float default_flt_val);
    float ReadFlt(XML_NODE* start_node, pcstr path, int index, float default_flt_val);
    float ReadFlt(XML_NODE* node, float default_flt_val);

    pcstr ReadAttrib(pcstr path, int index, pcstr attrib, pcstr default_str_val = "");
    pcstr ReadAttrib(XML_NODE* start_node, pcstr path, int index, pcstr attrib, pcstr default_str_val = "");
    pcstr ReadAttrib(XML_NODE* node, pcstr attrib, pcstr default_str_val);

    int ReadAttribInt(pcstr path, int index, pcstr attrib, int default_int_val = 0);
    int ReadAttribInt(XML_NODE* start_node, pcstr path, int index, pcstr attrib, int default_int_val = 0);
    int ReadAttribInt(XML_NODE* node, pcstr attrib, int default_int_val);

    float ReadAttribFlt(pcstr path, int index, pcstr attrib, float default_flt_val = 0.0f);
    float ReadAttribFlt(XML_NODE* start_node, pcstr path, int index, pcstr attrib, float default_flt_val = 0.0f);
    float ReadAttribFlt(XML_NODE* node, pcstr attrib, float default_flt_val = 0.0f);

    XML_NODE* SearchForAttribute(pcstr path, int index, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern);
    XML_NODE* SearchForAttribute(XML_NODE* start_node, pcstr tag_name, pcstr attrib, pcstr attrib_value_pattern);

    //возвращает количество узлов с заданым именем
    int GetNodesNum(pcstr path, int index, pcstr tag_name);
    int GetNodesNum(XML_NODE* node, pcstr tag_name);

#ifdef DEBUG // debug & mixed
    //проверка того, что аттрибуты у тегов уникальны
    //(если не NULL, то уникальность нарушена и возврашается имя
    //повторяющегося атрибута)
    pcstr CheckUniqueAttrib(XML_NODE* start_node, pcstr tag_name, pcstr attrib_name);
#endif

    //переместиться по XML дереву
    //путь задается в форме PARENT:CHILD:CHIDLS_CHILD
    // node_index - номер, если узлов с одним именем несколько
    XML_NODE* NavigateToNode(pcstr path, int node_index = 0);
    XML_NODE* NavigateToNode(XML_NODE* start_node, pcstr path, int node_index = 0);
    XML_NODE* NavigateToNodeWithAttribute(pcstr tag_name, pcstr attrib_name, pcstr attrib_value);

    void SetLocalRoot(XML_NODE* pLocalRoot) { m_pLocalRoot = pLocalRoot; }
    XML_NODE* GetLocalRoot() { return m_pLocalRoot; }
    XML_NODE* GetRoot() { return m_root; }
protected:
    XML_NODE* m_root;
    XML_NODE* m_pLocalRoot;

#ifdef DEBUG // debug & mixed
    //буфферный вектор для проверки уникальность аттрибутов
    xr_vector<shared_str> m_AttribValues;
#endif
public:
    virtual shared_str correct_file_name(pcstr path, pcstr fn) { return fn; }
private:
    XMLDocument(const XMLDocument& copy);
    void operator=(const XMLDocument& copy);

    typedef TiXmlElement XML_ELEM;
    TiXmlDocument m_Doc;
};

#endif // xrXMLParserH
