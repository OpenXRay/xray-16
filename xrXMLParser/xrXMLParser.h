#ifndef xrXMLParserH
#define xrXMLParserH
#pragma once


#ifdef XRXMLPARSER_EXPORTS
	#define XRXMLPARSER_API __declspec(dllexport)
#else
	#define XRXMLPARSER_API __declspec(dllimport)
	#pragma comment			(lib,"xrXMLParser.lib")
#endif

const LPCSTR CONFIG_PATH			= "$game_config$";
const LPCSTR UI_PATH				= "ui";


#include "tinyxml.h"

typedef TiXmlNode		XML_NODE;
typedef TiXmlAttribute	XML_ATTRIBUTE;

class XRXMLPARSER_API	CXml  
{
	void 				Load					(LPCSTR path_alias, LPCSTR xml_filename);
public:
	string_path			m_xml_file_name;
						CXml					();
	virtual				~CXml					();
	void				ClearInternal			();

	void 				Load					(LPCSTR path_alias, LPCSTR path, LPCSTR  xml_filename);


	//чтение элементов
	LPCSTR 				Read					(LPCSTR path, int index,  LPCSTR   default_str_val);
	LPCSTR 				Read					(XML_NODE* start_node, LPCSTR path, int index,  LPCSTR   default_str_val );
	LPCSTR 				Read					(XML_NODE* node,  LPCSTR   default_str_val);

	int   				ReadInt					(LPCSTR path, int index,  int default_int_val);
	int   				ReadInt					(XML_NODE* start_node,  LPCSTR path, int index,  int default_int_val);
	int   				ReadInt					(XML_NODE* node,  int default_int_val);

	float   			ReadFlt					(LPCSTR path, int index,  float default_flt_val);
	float   			ReadFlt					(XML_NODE* start_node,  LPCSTR path, int index,  float default_flt_val);
	float   			ReadFlt					(XML_NODE* node,  float default_flt_val);


	LPCSTR				ReadAttrib				(LPCSTR path,  int index, LPCSTR attrib, LPCSTR default_str_val = "");
	LPCSTR				ReadAttrib				(XML_NODE* start_node, LPCSTR path,  int index, LPCSTR attrib, LPCSTR default_str_val = "");
	LPCSTR				ReadAttrib				(XML_NODE* node, LPCSTR attrib, LPCSTR default_str_val);

	int					ReadAttribInt			(LPCSTR path, int index, LPCSTR attrib, int default_int_val = 0);
	int					ReadAttribInt			(XML_NODE* start_node, LPCSTR path, int index, LPCSTR attrib, int default_int_val = 0);
	int					ReadAttribInt			(XML_NODE* node, LPCSTR attrib, int default_int_val);

	float   			ReadAttribFlt			(LPCSTR path,	int index,  LPCSTR attrib, float default_flt_val = 0.0f);
	float   			ReadAttribFlt			(XML_NODE* start_node, LPCSTR path, int index,  LPCSTR attrib, float default_flt_val = 0.0f);
	float   			ReadAttribFlt			(XML_NODE* node,	LPCSTR attrib, float default_flt_val = 0.0f);

	XML_NODE*			SearchForAttribute		(LPCSTR path, int index, LPCSTR tag_name, LPCSTR attrib, LPCSTR attrib_value_pattern);
	XML_NODE*			SearchForAttribute		(XML_NODE* start_node, LPCSTR tag_name, LPCSTR attrib, LPCSTR attrib_value_pattern);

	//возвращает количество узлов с заданым именем
	int					GetNodesNum				(LPCSTR path, int index, LPCSTR tag_name);
	int					GetNodesNum				(XML_NODE* node, LPCSTR  tag_name);


#ifdef DEBUG // debug & mixed
	//проверка того, что аттрибуты у тегов уникальны
	//(если не NULL, то уникальность нарушена и возврашается имя 
	//повторяющегося атрибута)
	LPCSTR				CheckUniqueAttrib		(XML_NODE* start_node, LPCSTR tag_name, LPCSTR attrib_name);
#endif

	//переместиться по XML дереву 
	//путь задается в форме PARENT:CHILD:CHIDLS_CHILD
	//node_index - номер, если узлов с одним именем несколько
	XML_NODE*			NavigateToNode			(LPCSTR  path, int node_index = 0);
	XML_NODE*			NavigateToNode			(XML_NODE* start_node, LPCSTR  path, int node_index = 0);
	XML_NODE*			NavigateToNodeWithAttribute(LPCSTR tag_name, LPCSTR attrib_name, LPCSTR attrib_value);

	void				SetLocalRoot			(XML_NODE* pLocalRoot)		 	{m_pLocalRoot = pLocalRoot;}
	XML_NODE*			GetLocalRoot			()							 	{return m_pLocalRoot;}

	XML_NODE*			GetRoot					() {return m_root;}

protected:
	XML_NODE*			m_root;
	XML_NODE*			m_pLocalRoot;

#ifdef DEBUG // debug & mixed
	//буфферный вектор для проверки уникальность аттрибутов
	xr_vector<shared_str> m_AttribValues;
#endif
public:
	virtual shared_str correct_file_name		(LPCSTR path, LPCSTR fn) {return fn;}
private:
						CXml					(const CXml& copy);
	void				operator=				( const CXml& copy );

	typedef TiXmlElement						XML_ELEM;
	TiXmlDocument								m_Doc;
};

#endif //xrXMLParserH