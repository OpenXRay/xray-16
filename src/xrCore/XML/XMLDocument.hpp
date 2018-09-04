#pragma once
#ifndef xrXMLParserH
#define xrXMLParserH

#if !XRAY_EXCEPTIONS
#define PUGIXML_NO_EXCEPTIONS
#endif
#define PUGIXML_NO_XPATH
#define PUGIXML_HAS_LONG_LONG
#include "pugixml.hpp"
#ifdef DEBUG // debug & mixed
#include "xrCommon/xr_vector.h"
#endif
#include "xrCore/xrstring.h"

// XXX: interesting idea is to have variable configs folder. Need we?
static constexpr pcstr CONFIG_PATH = _game_config_;

class XML_NODE
{
    pugi::xml_node node;

public:
    XML_NODE() : node() {}

    explicit XML_NODE(const pugi::xml_node node)
        : node(node) {}

    operator bool() const
    {
        return node;
    }

    bool operator==(nullptr_t) = delete;

    XML_NODE firstChild() const
    {
        return XML_NODE(node.first_child());
    }

    XML_NODE firstChild(pcstr name) const
    {
        return XML_NODE(node.child(name));
    }

    XML_NODE nextSibling() const
    {
        return XML_NODE(node.next_sibling());
    }

    XML_NODE nextSibling(pcstr name) const
    {
        return XML_NODE(node.next_sibling(name));
    }

    pcstr textValueOr(pcstr defaultValue) const
    {
        const auto text = node.text();
        return text ? text.get() : defaultValue;
    }

    pcstr elementAttribute(pcstr name) const
    {
        if (node.type() == pugi::node_element)
        {
            const auto attr = node.attribute(name);
            return attr ? attr.value() : nullptr;
        }
        return nullptr;
    }

    pcstr elementValue() const
    {
        if (node.type() == pugi::node_element)
            return node.name();

        return nullptr;
    }

    pcstr value() const
    {
        switch (node.type())
        {
        case pugi::node_element:
            return node.name();
        default:
            return node.value();
        }
    }
};

struct XML_DOC
{
    pugi::xml_document doc;
    pugi::xml_parse_result res;

    void clear()
    {
        doc.reset();
    }

    void parse(pcstr data)
    {
        res = doc.load_string(data);
    }

    bool isError() const
    {
        return !res;
    }

    pcstr error() const
    {
        return res.description();
    }

    size_t errorOffset() const
    {
        return res.offset;
    }

    XML_NODE firstChildElement() const
    {
        return XML_NODE(doc.document_element());
    }
};

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

#ifdef DEBUG // debug & mixed
    //проверка того, что аттрибуты у тегов уникальны
    //(если не NULL, то уникальность нарушена и возврашается имя
    //повторяющегося атрибута)
    pcstr CheckUniqueAttrib(XML_NODE start_node, pcstr tag_name, pcstr attrib_name);
#endif

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

#ifdef DEBUG // debug & mixed
    //буфферный вектор для проверки уникальность аттрибутов
    xr_vector<shared_str> m_AttribValues;
#endif
public:
    virtual shared_str correct_file_name(pcstr path, pcstr fn) { return fn; }

private:
    XMLDocument(const XMLDocument& copy);
    void operator=(const XMLDocument& copy);

    XML_DOC m_Doc;
};

#endif // xrXMLParserH
