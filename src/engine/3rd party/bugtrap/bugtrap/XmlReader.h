/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: XML parser.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "TextFormat.h"
#include "InputStream.h"
#include "Array.h"
#include "Hash.h"
#include "StrStream.h"
#include "StrHolder.h"
#include "Encoding.h"

/// XML parser.
class CXmlReader
{
private:
	/// Forward declaration of XML node class required for the friendship.
	class CXmlParser;

public:
	/// Forward declaration of XML node class required for the friendship.
	class CXmlNode;

	/// List of attributes.
	class CAttributesList : private CHash<CStrStream, CStrStream>
	{
		friend class CXmlNode;
		friend class CXmlParser;
	private:
		/// Useful shorthand.
		typedef CHash<CStrStream, CStrStream> CAttributesListBase;
		/// Perform case-insensitive attribute search.
		PCTSTR CaselessGetAt(PCTSTR pszName) const;

	public:
		/// Synonym of hash iterator type.
		typedef CAttributesListBase::POSITION POSITION;

		/// Return number of items in collection.
		int GetCount(void) const;
		/// Tests for the empty-hash condition (no elements).
		bool IsEmpty(void) const;
		/// Returns attribute value for a given name.
		PCTSTR GetAt(PCTSTR pszName) const;
		/// Returns attribute value for a given name.
		PCTSTR operator[](PCTSTR pszName) const;
		/// Returns the position of the first element.
		POSITION GetStartPosition(void) const;
		/// Gets the next element for iterating.
		POSITION GetNextPosition(const POSITION& pos) const;
		/// Get attribute name at a given position.
		static PCTSTR GetNameAt(const POSITION& pos);
		/// Get attribute value a given position.
		static PCTSTR GetValueAt(const POSITION& pos);
	};

	/// Describes current XML node.
	class CXmlNode
	{
		friend class CXmlParser;
		friend class CXmlReader;
	public:
		/// Type of the node.
		enum XML_NODE_TYPE
		{
			/// Undefined node type.
			XNT_UNDEFINED,
			/// Complete element.
			XNT_ELEMENT,
			/// Start of the element.
			XNT_ELEMENT_BEGIN,
			/// End of the element.
			XNT_ELEMENT_END,
			/// Comment text.
			XNT_COMMENT,
			/// Embedded text.
			XNT_TEXT,
			/// CDATA text.
			XNT_CDATA,
			/// Processing instruction.
			XNT_PROCINSTR
		};

		/// Initialize the object.
		CXmlNode(void);
		/// Get node type.
		XML_NODE_TYPE GetNodeType(void) const;
		/// Get node name.
		PCTSTR GetNodeName(void) const;
		/// Get node value.
		PCTSTR GetNodeValue(void) const;
		/// Get map of node attributes.
		const CAttributesList& GetAttributes(void) const;
		/// Reset node info.
		void Reset(void);

	private:
		/// Node type.
		XML_NODE_TYPE m_eNodeType;
		/// Node name (if applicable).
		CStrStream m_strNodeName;
		/// Node value (if applicable).
		CStrStream m_strNodeValue;
		/// Attributes and their values (if applicable).
		CAttributesList m_mapAttributes;
	};

	/// Content filter.
	enum XML_CONTENT_FILTER
	{
		/// Don't perform any special post-processing.
		XCF_NONE        = 0x00,
		/// Strip comments.
		XCF_COMMENT     = 0x01,
		/// Merge CDATA with the text.
		XCF_CDATA       = 0x02,
		/// Strip processing instructions.
		XCF_PROCINSTR   = 0x04,
		/// Strip surrounding white-spaces.
		XCF_WHITESPACES = 0x08,
		/// Combination of all post-processing flags.
		XCF_ALL         = 0xFF
	};

	/// XML go to flags.
	enum XML_GOTO_FLAGS
	{
		/// True if child elements are accepted.
		XGF_ALLOW_NESTED_ELEMENTS = 0x01,
		/// True if parent element could be ended.
		XGF_ALLOW_ELEMENT_END     = 0x02
	};

	/// Initialize the object.
	CXmlReader(void);
	/// Initialize the object.
	explicit CXmlReader(PCTSTR pszUrl);
	/// Initialize the object.
	explicit CXmlReader(CInputStream* pInputStream);
	/// Object destructor.
	~CXmlReader(void);
	/// Open external URL.
	bool Open(PCTSTR pszUrl);
	/// Reset object state.
	void Close(void);
	/// Attach input stream to the parser.
	void SetInputStream(CInputStream* pInputStream);
	/// Get pointer to the input stream.
	CInputStream* GetInputStream(void) const;
	/// Get error message.
	PCTSTR GetErrorMessage(void) const;
	/// Get current content filter.
	DWORD GetContentFilter(void) const;
	/// Apply new content filter.
	void SetContentFilter(DWORD dwContentFilter);
	/// Read next node from the stream.
	int ReadNext(CXmlNode& rXmlNode);
	/// Read next element from the stream, text or PI if necessary.
	int GotoNextElement(CXmlNode& rXmlNode, DWORD dwGotoFlags = XGF_ALLOW_ELEMENT_END);
	/// Goes to the end of the next element.
	int GotoNextElementEnd(CXmlNode& rXmlNode, DWORD dwGotoFlags = 0);
	/// Read next element from the stream, skip text or PI if necessary.
	int GotoNextElement(PCTSTR pszName, CXmlNode& rXmlNode, DWORD dwGotoFlags = XGF_ALLOW_ELEMENT_END);
	/// Goes to the element with the specified name.
	int GotoElement(PCTSTR pszName, CXmlNode& rXmlNode, DWORD dwGotoFlags = XGF_ALLOW_ELEMENT_END);
	/// Read next text node by enforcing constants on node type, skip PI if necessary.
	int GotoText(CXmlNode& rXmlNode, DWORD dwGotoFlags = XGF_ALLOW_ELEMENT_END);
	/// Initialize standard entities.
	static void InitStdEntities(void);
	/// Free standard entities.
	static void FreeStdEntries(void);

private:
	/// Object can't be copied.
	CXmlReader(const CXmlReader& rReader);
	/// Object can't be copied.
	CXmlReader& operator=(const CXmlReader& rReader);
	/// Initialize the object.
	void Init(void);
	/// Release input stream if necessary.
	void ReleaseInputStream(void);

	/// Adjacent mode.
	enum ADJACENT_MODE
	{
		/// Item can't be grouped.
		AM_NONE = 0x00,
		/// Text value group.
		AM_TEXT = 0x01
	};

	/// XML result codes.
	enum XML_RESULT
	{
		/// Parse error.
		XMLR_ERROR    = -1,
		/// End of file/stream.
		XMLR_EOF      =  0,
		/// End of sequence.
		XMLR_EOS      = +1,
		/// Continue.
		XMLR_CONTINUE = +2
	};

	/// Type of tag.
	enum TAG_TYPE
	{
		/// Element.
		TT_ELEMENT,
		/// Processing instruction.
		TT_PROCINSTR
	};

	/// XML input stream.
	class CXmlInputStream
	{
	public:
		/// Object constructor.
		CXmlInputStream(void);
		/// Object constructor.
		CXmlInputStream(CCharInputStream* pInputStream);
		/// Object destructor.
		~CXmlInputStream(void);
		/// Get input stream.
		CCharInputStream* GetInputStream(void) const;
		/// Set input stream.
		void SetInputStream(CCharInputStream* pInputStream);
		/// Get text decoder.
		CBaseDecoder* GetDecoder(void) const;
		/// Set text decoder.
		bool SetDecoder(CBaseDecoder* pDecoder);
		/// Get stream name.
		bool GetName(PTSTR pszName, int nNameSize) const;
		/// Read character from the stream.
		int ReadChar(TCHAR arrChar[2]);
		/// Return true if back buffer is empty.
		bool IsBackBufferEmpty(void) const;
		/// Get number of elements in a back buffer.
		int GetBackBufferLength(void) const;
		/// Reserve space for multiple characters.
		void PrepareBackBuffer(int nNumChars);
		/// Put character back to the buffer.
		void PutCharBack(TCHAR chValue);
		/// Put character back to the buffer.
		void PutCharBack(const TCHAR arrChar[2], int nCharSize);
		/// Put character back to the buffer.
		void PutCharsBack(const TCHAR* pChars, int nNumChars);
		/// Put character back to the buffer.
		void PutCharsBack(PCTSTR pszString);
		/// Put character back to the buffer.
		void PutCharsBack(const CStrStream& rStream);
		/// Put character back to the buffer.
		void UnsafePutCharBack(TCHAR chValue);
		/// Put character back to the buffer.
		void UnsafePutCharBack(const TCHAR arrChar[2], int nCharSize);
		/// Put character back to the buffer.
		void UnsafePutCharsBack(const TCHAR* pChars, int nNumChars);
		/// Put character back to the buffer.
		void UnsafePutCharsBack(PCTSTR pszString);
		/// Put character back to the buffer.
		void UnsafePutCharsBack(const CStrStream& rStream);
		/// Remove N characters from the buffer.
		void UnsafeCutChars(int nNumChars);
		/// Remove N characters from the buffer.
		void UnsafeSkipChars(int nNumChars);
		/// Write single character.
		void WriteChar(TCHAR chValue);
		/// Write single character.
		void WriteChar(const TCHAR arrChar[2], int nCharSize);
		/// Write multiple characters.
		void WriteChars(const TCHAR* pChars, int nNumChars);
		/// Write multiple characters.
		void WriteChars(PCTSTR pszString);
		/// Write multiple characters.
		void WriteChars(const CStrStream& rStream);
		/// Write single character.
		void UnsafeWriteChar(TCHAR chValue);
		/// Write single character.
		void UnsafeWriteChar(const TCHAR arrChar[2], int nCharSize);
		/// Write multiple characters.
		void UnsafeWriteChars(const TCHAR* pChars, int nNumChars);
		/// Write multiple characters.
		void UnsafeWriteChars(PCTSTR pszString);
		/// Write multiple characters.
		void UnsafeWriteChars(const CStrStream& rStream);
		/// Read and apply byte order mark.
		bool CheckEncoding(void);
		/// Read and apply byte order mark.
		bool CheckEncoding(TEXT_ENCODING eDefaultEncoding);

	private:
		/// Object can't be copied.
		CXmlInputStream(const CXmlInputStream& rXmlInputStream);
		/// Object can't be copied.
		CXmlInputStream& operator=(const CXmlInputStream& rXmlInputStream);
		/// Initialize back buffer.
		void InitBackBuffer(void);
		/// Read character from the back buffer.
		bool ReadCharFromBackBuffer(TCHAR& chValue);

		/// Input stream.
		CCharInputStream* m_pInputStream;
		/// Buffer that keeps characters put back.
		PTCHAR m_pBackBuffer;
		/// Read position in the back buffer.
		int m_nBackBufferReadPos;
		/// Write position in the back buffer.
		int m_nBackBufferWritePos;
		/// Size of back buffer.
		int m_nBackBufferSize;
	};

	/// Node handler prototype.
	typedef XML_RESULT (CXmlParser::*FNodeHandler)(CXmlNode& rXmlNode, PVOID pParam);

	/// Search parameters.
	struct CXmlSearchParams
	{
		/// Initialize the object.
		CXmlSearchParams(void);
		/// Initialize the object.
		CXmlSearchParams(PCTSTR pszSequence, FNodeHandler pfnHandler, DWORD dwAdjacentMode = AM_NONE);
		/// Pointer to the node handler.
		FNodeHandler m_pfnHandler;
		/// Node sequence.
		PCTSTR m_pszSequence;
		/// Adjacent mode flag.
		DWORD m_dwAdjacentMode;
		/// True during while node is being searched.
		bool m_bActive;
	};

	/// Search table.
	struct CXmlSearchTable
	{
		/// Initialize the object.
		CXmlSearchTable(void);
		/// Initialize the object.
		CXmlSearchTable(CXmlSearchParams arrSearchParams[], int nNumParams);
		/// Set search table.
		void SetSearchTable(CXmlSearchParams arrSearchParams[], int nNumParams);
		/// Pre-fetched node handler.
		FNodeHandler m_pfnPrefetchedHandler;
		/// Current adjacent mode flag.
		DWORD m_dwAdjacentMode;
		/// Array of search parameters.
		CXmlSearchParams* m_arrSearchParams;
		/// Number of parameters in the array.
		int m_nNumParams;
	};

	/// XML parser.
	class CXmlParser
	{
		friend class CXmlReader;
	public:
		/// Parser type.
		enum PARSER_TYPE
		{
			/// XML parser.
			PT_XML_DOCUMENT,
			/// DTD parser.
			PT_DTD_DOCUMENT
		};

		/// Object constructor.
		CXmlParser(PARSER_TYPE eParserType, CXmlReader& rReader, CXmlInputStream& rXmlInputStream);
		/// Object destructor.
		~CXmlParser(void);
		/// Create stream from the URL.
		CInputStream* CreateInputStream(PCTSTR pszUrl);
		/// Get error message.
		PCTSTR GetErrorMessage(void) const;
		/// Read next XML node.
		XML_RESULT ReadNextXmlNode(CXmlNode& rXmlNode);
		/// Read next DTD node.
		XML_RESULT ReadNextDtdNode(CXmlNode& rXmlNode);
		/// Read all DTD nodes.
		XML_RESULT ReadDtdNodes(bool bProcessingEnabled);
		/// Read all DTD nodes.
		XML_RESULT ReadDtdNodes(void);
		/// Apply parent content filter.
		void ApplyContentFilter(void);

	private:
		/// State of the parser.
		enum PARSER_STATE
		{
			/// Undefined XML parser state.
			PS_XML_UNDEFINED,
			/// XML prologue.
			PS_XML_PROLOGUE,
			/// XML document.
			PS_XML_DOCUMENT,
			/// XML epilogue.
			PS_XML_EPILOGUE,
			/// Undefined DTD parser state.
			PS_DTD_UNDEFINED,
			/// External DTD document.
			PS_DTD_DOCUMENT,
			/// Embedded DTD text.
			PS_EMBEDDED_DTD
		};

		/// Object can't be copied.
		CXmlParser(const CXmlParser& rParser);
		/// Object can't be copied.
		CXmlParser& operator=(const CXmlParser& rParser);
		/// Destroy search tables.
		void DeleteSearchTables(void);
		/// Read character from the stream.
		int ReadChar(TCHAR arrChar[2]);
		/// Get next character from the buffer.
		int ReadChar(TCHAR arrChar[2], bool bRequired);
		/// Put character to the stream.
		void PutCharToStream(CStrStream& rOutputStream, TCHAR chValue);
		/// Put character to the stream.
		void PutCharToStream(CStrStream& rOutputStream, const TCHAR arrChar[2], int nCharSize);
		/// Put characters to the stream.
		void PutCharsToStream(CStrStream& rOutputStream, const TCHAR* pChars, int nNumChars);
		/// Reserve space for multiple characters.
		void PrepareBackBuffer(int nNumChars);
		/// Put character back to the buffer.
		void PutCharBack(TCHAR chValue);
		/// Put character back to the buffer.
		void PutCharBack(const TCHAR arrChar[2], int nCharSize);
		/// Put character back to the buffer.
		void PutCharsBack(const TCHAR* pChars, int nNumChars);
		/// Put character back to the buffer.
		void PutCharsBack(PCTSTR pszString);
		/// Put character back to the buffer.
		void PutCharsBack(const CStrStream& rStream);
		/// Put character back to the buffer.
		void UnsafePutCharBack(TCHAR chValue);
		/// Put character back to the buffer.
		void UnsafePutCharBack(const TCHAR arrChar[2], int nCharSize);
		/// Put character back to the buffer.
		void UnsafePutCharsBack(const TCHAR* pChars, int nNumChars);
		/// Put character back to the buffer.
		void UnsafePutCharsBack(PCTSTR pszString);
		/// Put character back to the buffer.
		void UnsafePutCharsBack(const CStrStream& rStream);
		/// Get next non-space character from the buffer.
		int ReadNonSpaceChar(TCHAR arrChar[2]);
		/// Get entity character.
		TCHAR GetEntityChar(void) const;
		/// Get entities map.
		const CHash<CStrStream, CStrStream>* GetEntities(void) const;
		/// Get entities map.
		CHash<CStrStream, CStrStream>* GetEntities(void);
		/// Return true when document end can be reached.
		bool IsEndOfDocument() const;
		/// Skip next spaces from the buffer.
		XML_RESULT SkipSpaces(bool bRequired);
		/// Skip data until terminator sequence.
		XML_RESULT SkipUntil(PCTSTR pszTerminator);
		/// Read data until terminator sequence.
		XML_RESULT ReadUntil(CStrStream& rOutputStream, PCTSTR pszTerminator);
		/// Read data until terminator sequence.
		XML_RESULT ReadUntilEx(CStrStream* pOutputStream, PCTSTR pszTerminator);
		/// Read identifier.
		XML_RESULT ReadDecNumber(DWORD& dwNumber);
		/// Read identifier.
		XML_RESULT ReadHexNumber(DWORD& dwNumber);
		/// Read identifier.
		XML_RESULT ReadNameEx(CStrStream* pOutputStream);
		/// Read identifier.
		XML_RESULT ReadName(CStrStream& rOutputStream);
		/// Skip identifier.
		XML_RESULT SkipName(void);
		/// Read string.
		XML_RESULT ReadStringEx(CStrStream* pOutputStream);
		/// Read string.
		XML_RESULT ReadString(CStrStream& rOutputStream);
		/// Skip string.
		XML_RESULT SkipString(void);
		/// Read numeric entity.
		XML_RESULT ReadNumericEntity(int& nValueLength);
		/// Read named entity.
		XML_RESULT ReadNamedEntity(int& nValueLength);
		/// Read entity.
		XML_RESULT ReadEntity(int& nValueLength);
		/// Read text.
		XML_RESULT ReadText(CStrStream& rOutputStream, PCTSTR pszTerminators, bool bInAttribute);
		/// Read text.
		XML_RESULT ReadTextEx(CStrStream* pOutputStream, PCTSTR pszTerminators, bool bInAttribute);
		/// Read next node from the document.
		XML_RESULT ProcessNode(CXmlNode& rXmlNode, CXmlSearchTable& rSearchTable, PVOID pParam);
		/// Read next node from the document.
		XML_RESULT ReadNextNode(CXmlNode& rXmlNode, CXmlSearchTable& rSearchTable, PVOID pParam);
		/// Used to return error code for invalid tokens and to protect parser from freezing.
		XML_RESULT InvalidTokenHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// XML text handler.
		XML_RESULT XmlTextHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// DTD text handler.
		XML_RESULT DtdTextHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Comment handler.
		XML_RESULT CommentHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Element handler.
		XML_RESULT XmlElementHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Processing instruction handler.
		XML_RESULT ProcInstrHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Generic tag handler.
		XML_RESULT TagHandler(CXmlNode& rXmlNode, TAG_TYPE eTagType);
		/// CDATA section handler.
		XML_RESULT XmlCDataHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// DOCTYPE declaration handler.
		XML_RESULT DocTypeHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// DTD entity handler.
		XML_RESULT DtdEntityHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Conditional section handler.
		XML_RESULT DtdCondtionHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// End of conditional section handler.
		XML_RESULT DtdCondtionEndHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// End of DTD section handler.
		XML_RESULT DtdEndHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Read external ID.
		XML_RESULT ReadExternalName(CStrStream& rExternalName);
		/// DTD declaration handler.
		XML_RESULT DtdDeclarationHandler(CXmlNode& rXmlNode, PVOID pParam);
		/// Read next DTD node.
		XML_RESULT ReadNextDtdNode(CXmlNode& rXmlNode, bool bProcessingEnabled);

		/// XML search table.
		static CXmlSearchParams m_arrInitialXmlSearchParams[];
		/// DTD search table.
		static CXmlSearchParams m_arrInitialDtdSearchParams[];
		/// XML search table.
		CXmlSearchParams* m_arrXmlSearchParams;
		/// DTD search table.
		CXmlSearchParams* m_arrDtdSearchParams;
		/// XML search table.
		CXmlSearchTable m_XmlSearchTable;
		/// DTD search table.
		CXmlSearchTable m_DtdSearchTable;
		/// Parent XML reader.
		CXmlReader& m_rReader;
		/// XML input stream.
		CXmlInputStream& m_rXmlInputStream;
		/// Error message text.
		PCTSTR m_pszErrorMessage;
		/// Parser state.
		PARSER_STATE m_eParserState;
		/// Number of conditional entries.
		int m_nNumEntries;
	};

	/// Map of XML entities and their values.
	CHash<CStrStream, CStrStream> m_mapXmlEntities;
	/// Map of DTD entities and their values.
	CHash<CStrStream, CStrStream> m_mapDtdEntities;
	/// Map of standard entities and their values.
	static CHash<PCTSTR, TCHAR> m_mapStdEntities;
	/// Stack of open elements.
	CArray<CStrHolder> m_arrOpenElements;
	/// Error message text.
	PCTSTR m_pszErrorMessage;
	/// Content filter.
	DWORD m_dwContentFilter;
	/// Decoding stream.
	CDecInputStream m_DecStream;
	/// Main XML input stream.
	CXmlInputStream m_XmlInputStream;
	/// Main document parser.
	CXmlParser m_XmlParser;
	/// True if input stream must be deallocated.
	bool m_bReleaseInputStream;
};

inline CXmlReader::~CXmlReader(void)
{
	ReleaseInputStream();
}

inline CXmlReader::CXmlNode::CXmlNode(void)
	: m_strNodeName(32), m_strNodeValue(128)
{
	m_eNodeType = XNT_UNDEFINED;
}

/**
 * @return node type.
 */
inline CXmlReader::CXmlNode::XML_NODE_TYPE CXmlReader::CXmlNode::GetNodeType(void) const
{
	return m_eNodeType;
}

/**
 * @return node name.
 */
inline PCTSTR CXmlReader::CXmlNode::GetNodeName(void) const
{
	return (PCTSTR)m_strNodeName;
}

/**
 * @return node value.
 */
inline PCTSTR CXmlReader::CXmlNode::GetNodeValue(void) const
{
	return (PCTSTR)m_strNodeValue;
}

/**
 * @return node attributes.
 */
inline const CXmlReader::CAttributesList& CXmlReader::CXmlNode::GetAttributes(void) const
{
	return m_mapAttributes;
}

/**
 * @return pointer to input stream.
 */
inline CInputStream* CXmlReader::GetInputStream(void) const
{
	return m_DecStream.GetInputStream();
}

/**
 * @return current content filter.
 */
inline DWORD CXmlReader::GetContentFilter(void) const
{
	return m_dwContentFilter;
}

/**
 * @param dwContentFilter - new content filter.
 */
inline void CXmlReader::SetContentFilter(DWORD dwContentFilter)
{
	m_dwContentFilter = dwContentFilter;
	m_XmlParser.ApplyContentFilter();
}

/**
 * @return error message text.
 */
inline PCTSTR CXmlReader::GetErrorMessage(void) const
{
	return m_pszErrorMessage;
}

/**
 * @return error message text.
 */
inline PCTSTR CXmlReader::CXmlParser::GetErrorMessage(void) const
{
	return m_pszErrorMessage;
}

/**
 * @param chValue - character.
 */
inline void CXmlReader::CXmlInputStream::WriteChar(TCHAR chValue)
{
	PrepareBackBuffer(1);
	UnsafeWriteChar(chValue);
}

/**
 * @param arrChar - character.
 * @param nCharSize - size of character.
 */
inline void CXmlReader::CXmlInputStream::WriteChar(const TCHAR arrChar[2], int nCharSize)
{
	PrepareBackBuffer(nCharSize);
	UnsafeWriteChar(arrChar, nCharSize);
}

/**
 * @param chValue - character.
 */
inline void CXmlReader::CXmlInputStream::PutCharBack(TCHAR chValue)
{
	PrepareBackBuffer(1);
	UnsafePutCharBack(chValue);
}

/**
 * @param arrChar - character.
 * @param nCharSize - size of character.
 */
inline void CXmlReader::CXmlInputStream::PutCharBack(const TCHAR arrChar[2], int nCharSize)
{
	PrepareBackBuffer(nCharSize);
	UnsafePutCharBack(arrChar, nCharSize);
}

/**
 * @param pChars - array of characters.
 * @param nNumChars - number of characters.
 */
inline void CXmlReader::CXmlInputStream::WriteChars(const TCHAR* pChars, int nNumChars)
{
	PrepareBackBuffer(nNumChars);
	UnsafeWriteChars(pChars, nNumChars);
}

/**
 * @param pChars - array of characters.
 * @param nNumChars - number of characters.
 */
inline void CXmlReader::CXmlInputStream::PutCharsBack(const TCHAR* pChars, int nNumChars)
{
	PrepareBackBuffer(nNumChars);
	UnsafePutCharsBack(pChars, nNumChars);
}

/**
 * @param rOutputStream - output stream.
 * @param chValue - character.
 */
inline void CXmlReader::CXmlParser::PutCharToStream(CStrStream& rOutputStream, TCHAR chValue)
{
	rOutputStream << chValue;
}

/**
 * @param rOutputStream - output stream.
 * @param arrChar - character.
 * @param nCharSize - size of character in bytes.
 */
inline void CXmlReader::CXmlParser::PutCharToStream(CStrStream& rOutputStream, const TCHAR arrChar[2], int nCharSize)
{
	_ASSERTE(nCharSize > 0 && nCharSize <= 2);
	rOutputStream << arrChar[0];
	if (nCharSize > 1)
		rOutputStream << arrChar[1];
}

/**
 * @param rOutputStream - output stream.
 * @param pChars - characters array.
 * @param nNumChars - number of characters in the array.
 */
inline void CXmlReader::CXmlParser::PutCharsToStream(CStrStream& rOutputStream, const TCHAR* pChars, int nNumChars)
{
	for (int nCharNum = 0; nCharNum < nNumChars; ++nCharNum)
		rOutputStream << pChars[nCharNum];
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::XmlElementHandler(CXmlNode& rXmlNode, PVOID /*pParam*/)
{
	return TagHandler(rXmlNode, TT_ELEMENT);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ProcInstrHandler(CXmlNode& rXmlNode, PVOID /*pParam*/)
{
	return TagHandler(rXmlNode, TT_PROCINSTR);
}

/**
 * @return number of items in the collection.
 */
inline int CXmlReader::CAttributesList::GetCount(void) const
{
	return CAttributesListBase::GetCount();
}

/**
 * @return true if the collection is empty.
 */
inline bool CXmlReader::CAttributesList::IsEmpty(void) const
{
	return CAttributesListBase::IsEmpty();
}

/**
 * @param pszName - attribute name.
 * @return attribute value.
 */
inline PCTSTR CXmlReader::CAttributesList::GetAt(PCTSTR pszName) const
{
	const CStrStream* pStrStream = CAttributesListBase::Lookup(CStrStream(pszName));
	return (pStrStream != NULL ? (PCTSTR)*pStrStream : NULL);
}

/**
 * @param pszName - attribute name.
 * @return attribute value.
 */
inline PCTSTR CXmlReader::CAttributesList::operator[](PCTSTR pszName) const
{
	return GetAt(pszName);
}

/**
 * @return start attribute position.
 */
inline CXmlReader::CAttributesList::POSITION CXmlReader::CAttributesList::GetStartPosition(void) const
{
	return CAttributesListBase::GetStartPosition();
}

/**
 * @param pos - current attribute position.
 * @return next attribute position.
 */
inline CXmlReader::CAttributesList::POSITION CXmlReader::CAttributesList::GetNextPosition(const POSITION& pos) const
{
	return CAttributesListBase::GetNextPosition(pos);
}

/**
 * @param pos - current attribute position.
 * @return attribute name.
 */
inline PCTSTR CXmlReader::CAttributesList::GetNameAt(const POSITION& pos)
{
	return (PCTSTR)CAttributesListBase::GetKeyAt(pos);
}

/**
 * @param pos - current attribute position.
 * @return attribute value.
 */
inline PCTSTR CXmlReader::CAttributesList::GetValueAt(const POSITION& pos)
{
	return (PCTSTR)CAttributesListBase::GetDataAt(pos);
}

/**
 * @return true if back buffer is empty.
 */
inline bool CXmlReader::CXmlInputStream::IsBackBufferEmpty(void) const
{
	return (m_nBackBufferReadPos == m_nBackBufferWritePos);
}

/**
 * @return number of characters in a back buffer.
 */
inline int CXmlReader::CXmlInputStream::GetBackBufferLength(void) const
{
	return (m_nBackBufferReadPos <= m_nBackBufferWritePos ?
				m_nBackBufferWritePos - m_nBackBufferReadPos :
				m_nBackBufferSize - m_nBackBufferReadPos + m_nBackBufferWritePos);
}

/**
 * @param nNumChars - number of characters to cut.
 */
inline void CXmlReader::CXmlInputStream::UnsafeCutChars(int nNumChars)
{
	_ASSERTE(GetBackBufferLength() >= nNumChars);
	m_nBackBufferWritePos = (m_nBackBufferWritePos - nNumChars + m_nBackBufferSize) % m_nBackBufferSize;
}

/**
 * @param nNumChars - number of characters to cut.
 */
inline void CXmlReader::CXmlInputStream::UnsafeSkipChars(int nNumChars)
{
	_ASSERTE(GetBackBufferLength() >= nNumChars);
	m_nBackBufferReadPos += nNumChars;
	m_nBackBufferReadPos %= m_nBackBufferSize;
}

/**
 * @param chValue - character value.
 */
inline void CXmlReader::CXmlInputStream::UnsafeWriteChar(TCHAR chValue)
{
	_ASSERTE(GetBackBufferLength() < m_nBackBufferSize);
	m_pBackBuffer[m_nBackBufferWritePos] = chValue;
	m_nBackBufferWritePos = (m_nBackBufferWritePos + 1) % m_nBackBufferSize;
}

/**
 * @param arrChar - character.
 * @param nCharSize - size of character.
 */
inline void CXmlReader::CXmlInputStream::UnsafeWriteChar(const TCHAR arrChar[2], int nCharSize)
{
	_ASSERTE(nCharSize >= 0 && nCharSize < 2);
	UnsafeWriteChar(arrChar[0]);
	if (nCharSize > 1)
		UnsafeWriteChar(arrChar[1]);
}

/**
 * @param chValue - character value.
 */
inline void CXmlReader::CXmlInputStream::UnsafePutCharBack(TCHAR chValue)
{
	_ASSERTE(GetBackBufferLength() < m_nBackBufferSize);
	m_nBackBufferReadPos = (m_nBackBufferReadPos - 1 + m_nBackBufferSize) % m_nBackBufferSize;
	m_pBackBuffer[m_nBackBufferReadPos] = chValue;
}

/**
 * @param arrChar - character.
 * @param nCharSize - size of character.
 */
inline void CXmlReader::CXmlInputStream::UnsafePutCharBack(const TCHAR arrChar[2], int nCharSize)
{
	_ASSERTE(nCharSize >= 0 && nCharSize < 2);
	if (nCharSize > 1)
		UnsafePutCharBack(arrChar[1]);
	UnsafePutCharBack(arrChar[0]);
}

/**
 * @param pChars - array of characters.
 * @param nNumChars - number of characters.
 */
inline void CXmlReader::CXmlInputStream::UnsafeWriteChars(const TCHAR* pChars, int nNumChars)
{
	for (int nCharPos = 0; nCharPos < nNumChars; ++nCharPos)
		UnsafeWriteChar(pChars[nCharPos]);
}

/**
 * @param pChars - array of characters.
 * @param nNumChars - number of characters.
 */
inline void CXmlReader::CXmlInputStream::UnsafePutCharsBack(const TCHAR* pChars, int nNumChars)
{
	for (int nCharPos = nNumChars - 1; nCharPos >= 0; --nCharPos)
		UnsafePutCharBack(pChars[nCharPos]);
}

/**
 * @param rOutputStream - reference to the output stream or @a NULL.
 * @param pszTerminator - terminator sequence.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadUntil(CStrStream& rOutputStream, PCTSTR pszTerminator)
{
	return ReadUntilEx(&rOutputStream, pszTerminator);
}

/**
 * @param pszTerminator - terminator sequence.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::SkipUntil(PCTSTR pszTerminator)
{
	return ReadUntilEx(NULL, pszTerminator);
}

/**
 * @param rOutputStream - output stream.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadName(CStrStream& rOutputStream)
{
	return ReadNameEx(&rOutputStream);
}

/**
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::SkipName(void)
{
	return ReadNameEx(NULL);
}

/**
 * @param rOutputStream - output stream.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadString(CStrStream& rOutputStream)
{
	return ReadStringEx(&rOutputStream);
}

/**
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::SkipString(void)
{
	return ReadStringEx(NULL);
}

/**
 * @param rOutputStream - output stream.
 * @param pszTerminators - list of text terminators.
 * @param bInAttribute - true when reading attribute value.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadText(CStrStream& rOutputStream, PCTSTR pszTerminators, bool bInAttribute)
{
	return ReadTextEx(&rOutputStream, pszTerminators, bInAttribute);
}

inline CXmlReader::CXmlInputStream::CXmlInputStream(void)
{
	InitBackBuffer();
	m_pInputStream = NULL;
}

/**
 * @param pInputStream - input stream.
 */
inline CXmlReader::CXmlInputStream::CXmlInputStream(CCharInputStream* pInputStream)
{
	InitBackBuffer();
	SetInputStream(pInputStream);
}

/**
 * @return input stream.
 */
inline CCharInputStream* CXmlReader::CXmlInputStream::GetInputStream(void) const
{
	return m_pInputStream;
}

inline CXmlReader::CXmlInputStream::~CXmlInputStream()
{
	delete[] m_pBackBuffer;
}

/**
 * @return pointer to current decoder or @a NULL.
 */
inline CBaseDecoder* CXmlReader::CXmlInputStream::GetDecoder(void) const
{
	_ASSERTE(m_pInputStream != NULL);
	return m_pInputStream->GetDecoder();
}

/**
 * @param pDecoder - pointer to the decoder or @a NULL.
 * @return true if decoding was successfully applied.
 */
inline bool CXmlReader::CXmlInputStream::SetDecoder(CBaseDecoder* pDecoder)
{
	_ASSERTE(m_pInputStream != NULL);
	return m_pInputStream->SetDecoder(pDecoder);
}

/**
 * @param pszName - stream name buffer.
 * @param nNameSize - size of stream name buffer.
 * @return true if name was retrieved.
 */
inline bool CXmlReader::CXmlInputStream::GetName(PTSTR pszName, int nNameSize) const
{
	_ASSERTE(m_pInputStream != NULL);
	return m_pInputStream->GetName(pszName, nNameSize);
}

/**
 * @param arrChar - character data.
 * @return number of characters in one symbol.
 */
inline int CXmlReader::CXmlParser::ReadChar(TCHAR arrChar[2])
{
	return m_rXmlInputStream.ReadChar(arrChar);
}

/**
 * Specifies number of characters to reserve space for.
 * @param nNumChars - number of characters to reserve space for.
 */
inline void CXmlReader::CXmlParser::PrepareBackBuffer(int nNumChars)
{
	m_rXmlInputStream.PrepareBackBuffer(nNumChars);
}

/**
 * @param chValue - character.
 */
inline void CXmlReader::CXmlParser::PutCharBack(TCHAR chValue)
{
	m_rXmlInputStream.PutCharBack(chValue);
}

/**
 * @param arrChar - character.
 * @param nCharSize - size of character.
 */
inline void CXmlReader::CXmlParser::PutCharBack(const TCHAR arrChar[2], int nCharSize)
{
	m_rXmlInputStream.PutCharBack(arrChar, nCharSize);
}

/**
 * @param pChars - character array.
 * @param nNumChars - array length.
 */
inline void CXmlReader::CXmlParser::PutCharsBack(const TCHAR* pChars, int nNumChars)
{
	m_rXmlInputStream.PutCharsBack(pChars, nNumChars);
}

/**
 * @param chValue - character.
 */
inline void CXmlReader::CXmlParser::UnsafePutCharBack(TCHAR chValue)
{
	m_rXmlInputStream.UnsafePutCharBack(chValue);
}

/**
 * @param arrChar - character.
 * @param nCharSize - size of character.
 */
inline void CXmlReader::CXmlParser::UnsafePutCharBack(const TCHAR arrChar[2], int nCharSize)
{
	m_rXmlInputStream.UnsafePutCharBack(arrChar, nCharSize);
}

/**
 * @param pChars - character array.
 * @param nNumChars - array length.
 */
inline void CXmlReader::CXmlParser::UnsafePutCharsBack(const TCHAR* pChars, int nNumChars)
{
	m_rXmlInputStream.UnsafePutCharsBack(pChars, nNumChars);
}

inline CXmlReader::CXmlSearchTable::CXmlSearchTable(void)
{
	SetSearchTable(NULL, 0);
}

/**
 * @param arrSearchParams - array of search parameters.
 * @param nNumParams - number of parameters in the array.
 */
inline CXmlReader::CXmlSearchTable::CXmlSearchTable(CXmlSearchParams arrSearchParams[], int nNumParams)
{
	SetSearchTable(arrSearchParams, nNumParams);
}

/**
 * @param pszString - null-terminated string.
 */
inline void CXmlReader::CXmlInputStream::PutCharsBack(PCTSTR pszString)
{
	PutCharsBack(pszString, _tcslen(pszString));
}

/**
 * @param rStream - string stream.
 */
inline void CXmlReader::CXmlInputStream::PutCharsBack(const CStrStream& rStream)
{
	PutCharsBack((PCTSTR)rStream, rStream.GetLength());
}

/**
 * @param pszString - null-terminated string.
 */
inline void CXmlReader::CXmlInputStream::UnsafePutCharsBack(PCTSTR pszString)
{
	UnsafePutCharsBack(pszString, _tcslen(pszString));
}

/**
 * @param rStream - string stream.
 */
inline void CXmlReader::CXmlInputStream::UnsafePutCharsBack(const CStrStream& rStream)
{
	UnsafePutCharsBack((PCTSTR)rStream, rStream.GetLength());
}

/**
 * @param pszString - null-terminated string.
 */
inline void CXmlReader::CXmlInputStream::WriteChars(PCTSTR pszString)
{
	WriteChars(pszString, _tcslen(pszString));
}

/**
 * @param rStream - string stream.
 */
inline void CXmlReader::CXmlInputStream::WriteChars(const CStrStream& rStream)
{
	WriteChars((PCTSTR)rStream, rStream.GetLength());
}

/**
 * @param pszString - null-terminated string.
 */
inline void CXmlReader::CXmlInputStream::UnsafeWriteChars(PCTSTR pszString)
{
	UnsafeWriteChars(pszString, _tcslen(pszString));
}

/**
 * @param rStream - string stream.
 */
inline void CXmlReader::CXmlInputStream::UnsafeWriteChars(const CStrStream& rStream)
{
	UnsafeWriteChars((PCTSTR)rStream, rStream.GetLength());
}

/**
 * @param pszString - null-terminated string.
 */
inline void CXmlReader::CXmlParser::PutCharsBack(PCTSTR pszString)
{
	m_rXmlInputStream.PutCharsBack(pszString);
}

/**
 * @param rStream - string stream.
 */
inline void CXmlReader::CXmlParser::PutCharsBack(const CStrStream& rStream)
{
	m_rXmlInputStream.PutCharsBack(rStream);
}

/**
 * @param pszString - null-terminated string.
 */
inline void CXmlReader::CXmlParser::UnsafePutCharsBack(PCTSTR pszString)
{
	m_rXmlInputStream.UnsafePutCharsBack(pszString);
}

/**
 * @param rStream - string stream.
 */
inline void CXmlReader::CXmlParser::UnsafePutCharsBack(const CStrStream& rStream)
{
	m_rXmlInputStream.UnsafePutCharsBack(rStream);
}

inline void CXmlReader::CXmlParser::DeleteSearchTables(void)
{
	delete[] m_arrXmlSearchParams;
	delete[] m_arrDtdSearchParams;
}

inline CXmlReader::CXmlParser::~CXmlParser(void)
{
	DeleteSearchTables();
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNextDtdNode(CXmlNode& rXmlNode)
{
	return ReadNextDtdNode(rXmlNode, true);
}

/**
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadDtdNodes(void)
{
	return ReadDtdNodes(true);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param bProcessingEnabled - true if DTD processing should be enabled.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNextDtdNode(CXmlNode& rXmlNode, bool bProcessingEnabled)
{
	return ReadNextNode(rXmlNode, m_DtdSearchTable, (PVOID)bProcessingEnabled);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @return XML result code.
 */
inline CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNextXmlNode(CXmlNode& rXmlNode)
{
	return ReadNextNode(rXmlNode, m_XmlSearchTable, NULL);
}

/**
 * @return true if stream encoding has been read from the stream.
 */
inline bool CXmlReader::CXmlInputStream::CheckEncoding()
{
	_ASSERTE(m_pInputStream != NULL);
	return m_pInputStream->CheckEncoding();
}

/**
 * @param eDefaultEncoding - default encoding.
 * @return true if byte order mark has been read from the stream.
 */
inline bool CXmlReader::CXmlInputStream::CheckEncoding(TEXT_ENCODING eDefaultEncoding)
{
	_ASSERTE(m_pInputStream != NULL);
	return m_pInputStream->CheckEncoding(eDefaultEncoding);
}

/**
 * @param rXmlNode - current node data is stored in this variable.
 * @return XML result code.
 */
inline int CXmlReader::ReadNext(CXmlNode& rXmlNode)
{
	int nResult = m_XmlParser.ReadNextXmlNode(rXmlNode);
	m_pszErrorMessage = m_XmlParser.m_pszErrorMessage;
	return nResult;
}

/**
 * @return true when document end can be reached.
 */
inline bool CXmlReader::CXmlParser::IsEndOfDocument() const
{
	return ((m_eParserState == PS_XML_EPILOGUE || m_eParserState == PS_DTD_DOCUMENT) && m_nNumEntries == 0);
}

inline void CXmlReader::FreeStdEntries(void)
{
	m_mapStdEntities.DeleteAll(true);
}
