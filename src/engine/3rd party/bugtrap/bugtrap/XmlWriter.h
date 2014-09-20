/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: XML generator.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "Encoding.h"
#include "Array.h"
#include "StrHolder.h"

/// XML generator.
class CXmlWriter
{
public:
	/// Initialize the object.
	CXmlWriter(void);
	/// Initialize the object.
	explicit CXmlWriter(COutputStream* pOutputStream);
	/// Set output stream.
	void SetOutputStream(COutputStream* pOutputStream);
	/// Get output stream.
	COutputStream* GetOutputStream(void) const;
	/// Get current indentation.
	void GetIndentation(CHAR& chIndentChar, DWORD& dwIndentation) const;
	/// Set indentation.
	void SetIndentation(CHAR chIndentChar, DWORD dwIndentation);
	/// Write attribute string.
	BOOL WriteAttributeString(PCTSTR pszLocalName, PCTSTR pszString);
	/// Encodes the specified binary bytes as Base64.
	BOOL WriteBase64(const BYTE* pBytes, DWORD dwNumBytes);
	/// Encodes the specified binary bytes as bin-hex.
	BOOL WriteBinHex(const BYTE* pBytes, DWORD dwNumBytes);
	/// Writes a <![CDATA[...]]> block containing the specified text.
	BOOL WriteCData(PCTSTR pszString);
	/// Forces the generation of a character entity for the specified Unicode character value.
	BOOL WriteCharEntity(TCHAR ch);
	/// Writes a comment <!--...--> containing the specified text.
	BOOL WriteComment(PCTSTR pszString);
	/// Writes the DOCTYPE declaration with the specified name and optional attributes.
	BOOL WriteDocType(PCTSTR pszName, PCTSTR pszPubID, PCTSTR pszSysID, PCTSTR pszSubset);
	/// Writes an element containing a string value.
	BOOL WriteElementString(PCTSTR pszLocalName, PCTSTR pszString);
	/// Closes the previous WriteStartAttribute() call.
	BOOL WriteEndAttribute(void);
	/// Close all open tags.
	BOOL WriteEndDocument(void);
	/// Write ending element tag.
	BOOL WriteEndElement(void);
	/// Writes an entity reference as &name;.
	BOOL WriteEntityRef(PCTSTR pszName);
	/// Write full ending element tag.
	BOOL WriteFullEndElement(void);
	/// Writes a processing instruction with a space between the name and text as follows: <?name text?>.
	BOOL WriteProcessingInstruction(PCTSTR pszLocalName, PCTSTR pszString);
	/// Writes raw markup manually.
	BOOL WriteRaw(PCTSTR pszString);
	/// Writes the start of an attribute.
	BOOL WriteStartAttribute(PCTSTR pszLocalName);
	/// Write document starting tag.
	BOOL WriteStartDocument(void);
	/// Write starting element tag.
	BOOL WriteStartElement(PCTSTR pszLocalName);
	/// Writes the given text content.
	BOOL WriteString(PCTSTR pszString);

private:
	/// Object can't be copied.
	CXmlWriter(const CXmlWriter& rWriter);
	/// Object can't be copied.
	CXmlWriter& operator=(const CXmlWriter& rWriter);

	/// Escape flags.
	enum ESCAPE_FLAGS
	{
		/// Don't perform any special escaping.
		EF_ESCAPENONE          = 0x00,
		/// Escape quotation marks.
		EF_ESCAPEQUOTMARKS     = 0x01,
		/// Escape non-ASCII characters.
		EF_ESCAPENONASCIICHARS = 0x02
	};

	/// Internal writer state.
	enum WRITER_STATE
	{
		/// No data.
		WS_NODATA,
		/// Constructing document tag section.
		WS_DOCUMENT,
		/// Constructing text section.
		WS_TEXT,
		/// Constructing element tag.
		WS_ELEMENT,
		/// Constructing attribute.
		WS_ATTRUBUTE
	};

	/// Initialize member variables.
	void InitVars(void);
	/// Write indent for a new tag.
	BOOL WriteTagIndent(void);
	/// Write indent in the beginning of the line.
	BOOL WriteIndent(void);
	/// Write new line characters.
	BOOL WriteNewLine(void);
	/// Write escaped string.
	BOOL WriteEscaped(PCTSTR pszString, DWORD dwEscapeFlags = EF_ESCAPENONE);
	/// Ensures that the array has enough space.
	BOOL EnsureSize(DWORD dwSize);
	/// Finalize element tag.
	BOOL FinalizeElement(void);

	/// Internal write sate.
	WRITER_STATE m_eWriterState;
	/// Stack of open elements.
	CArray<CStrHolder> m_arrOpenElements;
	/// Indentation value.
	DWORD m_dwIndentation;
	/// Indent character.
	CHAR m_chIndentChar;
	/// Topmost tag flag.
	BOOL m_bTopmostTag;
	/// UTF-8 encoder.
	CUTF8EncStream m_EncStream;
};

inline CXmlWriter::CXmlWriter(void)
{
	InitVars();
}

/**
 * @param pOutputStream - pointer to the output stream.
 */
inline CXmlWriter::CXmlWriter(COutputStream* pOutputStream) : m_EncStream(pOutputStream)
{
	InitVars();
}

/**
 * @param pOutputStream - pointer to the output stream.
 */
inline void CXmlWriter::SetOutputStream(COutputStream* pOutputStream)
{
	InitVars();
	m_EncStream.SetOutputStream(pOutputStream);
}

/**
 * @return binary buffer.
 */
inline COutputStream* CXmlWriter::GetOutputStream(void) const
{
	return m_EncStream.GetOutputStream();
}

/**
 * @param chIndentChar - indentation character.
 * @param dwIndentation - indentation value.
 */
inline void CXmlWriter::GetIndentation(CHAR& chIndentChar, DWORD& dwIndentation) const
{
	chIndentChar = m_chIndentChar;
	dwIndentation = m_dwIndentation;
}

/**
 * @param chIndentChar - indentation character.
 * @param dwIndentation - indentation value.
 */
inline void CXmlWriter::SetIndentation(CHAR chIndentChar, DWORD dwIndentation)
{
	m_chIndentChar = chIndentChar;
	m_dwIndentation = dwIndentation;
}

/**
 * @return true if data has been written.
 */
inline BOOL CXmlWriter::WriteNewLine(void)
{
	return (m_EncStream.WriteByte('\r') && m_EncStream.WriteByte('\n'));
}

/**
 * @return true if data has been successfully written.
 */
inline BOOL CXmlWriter::WriteFullEndElement(void)
{
	return (FinalizeElement() && WriteEndElement());
}
