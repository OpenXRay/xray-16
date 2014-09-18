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

#include "StdAfx.h"
#include "XmlWriter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CXmlWriter::InitVars(void)
{
	m_chIndentChar = '\0';
	m_dwIndentation = 0;
	m_eWriterState = WS_NODATA;
	m_bTopmostTag = FALSE;
}

/**
 * @param pBytes - bytes array.
 * @param dwNumBytes - array size.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteBinHex(const BYTE* pBytes, DWORD dwNumBytes)
{
	_ASSERTE(dwNumBytes == 0 || pBytes != NULL);
	_ASSERTE(m_eWriterState == WS_ELEMENT || m_eWriterState == WS_ATTRUBUTE);
	if (m_eWriterState != WS_ELEMENT && m_eWriterState != WS_ATTRUBUTE)
		return FALSE;
	static const BYTE s_arrHexChars[] =
	{
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
	for (DWORD dwByteNum = 0; dwByteNum < dwNumBytes; ++dwByteNum)
	{
		BYTE bValue = pBytes[dwByteNum];
		if (! m_EncStream.WriteByte((bValue >> 4) & 0x0F))
			return FALSE;
		if (! m_EncStream.WriteByte(bValue & 0x0F))
			return FALSE;
	}
	return TRUE;
}

/**
 * @param pszString - string to process.
 * @param dwEscapeFlags - escape flags.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteEscaped(PCTSTR pszString, DWORD dwEscapeFlags)
{
	_ASSERTE(pszString != NULL);
	DWORD dwPosition = 0;
	while (pszString[dwPosition] != _T('\0'))
	{
		switch (pszString[dwPosition])
		{
		case _T('<'):
			if (! m_EncStream.WriteAscii("&lt;"))
				return FALSE;
			++dwPosition;
			break;
		case _T('>'):
			if (! m_EncStream.WriteAscii("&gt;"))
				return FALSE;
			++dwPosition;
			break;
		case _T('&'):
			if (! m_EncStream.WriteAscii("&amp;"))
				return FALSE;
			++dwPosition;
			break;
		case _T('\''):
			if (dwEscapeFlags & EF_ESCAPEQUOTMARKS)
			{
				if (! m_EncStream.WriteAscii("&apos;"))
					return FALSE;
			}
			else
			{
				if (! m_EncStream.WriteByte('\''))
					return FALSE;
			}
			++dwPosition;
			break;
		case _T('\"'):
			if (dwEscapeFlags & EF_ESCAPEQUOTMARKS)
			{
				if (! m_EncStream.WriteAscii("&quot;"))
					return FALSE;
			}
			else
			{
				if (! m_EncStream.WriteByte('\"'))
					return FALSE;
			}
			++dwPosition;
			break;
		default:
			int nCharSize;
			if (dwEscapeFlags & EF_ESCAPENONASCIICHARS)
			{
				if (! m_EncStream.WriteAscii("&#x"))
					return FALSE;
				if (! m_EncStream.WriteUTF8Hex(pszString + dwPosition, nCharSize))
					return FALSE;
				if (! m_EncStream.WriteByte(';'))
					return FALSE;
			}
			else
			{
				if (! m_EncStream.WriteUTF8Bin(pszString + dwPosition, nCharSize))
					return FALSE;
			}
			dwPosition += nCharSize;
		}
	}
	return TRUE;
}

/**
 * @param pBytes - bytes array.
 * @param dwNumBytes - array size.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteBase64(const BYTE* pBytes, DWORD dwNumBytes)
{
	_ASSERTE(dwNumBytes == 0 || pBytes != NULL);
	_ASSERTE(m_eWriterState == WS_ELEMENT || m_eWriterState == WS_ATTRUBUTE);
	if (m_eWriterState != WS_ELEMENT && m_eWriterState != WS_ATTRUBUTE)
		return FALSE;
	if (dwNumBytes == 0)
		return TRUE;
	static const BYTE s_arrBase64EncodingTable[] =
	{
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',	'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
		'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
	};
	_ASSERTE(countof(s_arrBase64EncodingTable) == 64);
	BOOL bFormatOutput = m_chIndentChar != '\0' && m_eWriterState != WS_ATTRUBUTE;
	DWORD dwNumQuartets = dwNumBytes / 3 * 4; // Number of complete quartets
	DWORD dwNumLines = dwNumQuartets / 76; // Number of complete lines
	DWORD dwQuartetsPerLine = 76 / 4; // Number of quartets per line
	if (bFormatOutput)
		m_bTopmostTag = FALSE;
	for (DWORD dwLineNum = 0; dwLineNum <= dwNumLines; ++dwLineNum)
	{
		if (bFormatOutput && dwLineNum > 0 && ! WriteTagIndent())
			return FALSE;
		if (dwLineNum == dwNumLines)
			dwQuartetsPerLine = (dwNumQuartets % 76) / 4; // Number of complete quartets in the last line
		for (DWORD dwQuarterNum = 0; dwQuarterNum < dwQuartetsPerLine; ++dwQuarterNum)
		{
			DWORD dwAccumulator = 0;
			for (DWORD dwBytePos = 0; dwBytePos < 3; ++dwBytePos)
			{
				dwAccumulator |= *pBytes++;
				dwAccumulator <<= 8; // Note that lower 8 bits are always zero
			}
			for (DWORD dwBytePos = 0; dwBytePos < 4; ++dwBytePos)
			{
				BYTE bValue = (BYTE)(dwAccumulator >> 26); // Extract the following 6 bits from the 32-bit accumulator
				_ASSERTE(bValue >= 0 && bValue < countof(s_arrBase64EncodingTable));
				if (! m_EncStream.WriteByte(s_arrBase64EncodingTable[bValue]))
					return FALSE;
				dwAccumulator <<= 6; // Prepare next 6 bits
			}
		}
	}
	DWORD dwRestBytes = dwNumBytes % 3; // Number of un-encoded bytes in the last line
	if (dwRestBytes)
	{
		DWORD dwOutputBytes = dwRestBytes + 1; // N source bytes are encoded as N+1 bytes
		DWORD dwAccumulator = 0;
		for (DWORD dwBytePos = 0; dwBytePos < 3; ++dwBytePos)
		{
			if (dwBytePos < dwRestBytes)
				dwAccumulator |= *pBytes++;
			dwAccumulator <<= 8; // Note that at least lower 8 bits are always zero
		}
		for (DWORD dwBytePos = 0; dwBytePos < dwOutputBytes; ++dwBytePos)
		{
			BYTE bValue = (BYTE)(dwAccumulator >> 26); // Extract the following 6 bits from the 32-bit accumulator
			_ASSERTE(bValue >= 0 && bValue < countof(s_arrBase64EncodingTable));
			if (! m_EncStream.WriteByte(s_arrBase64EncodingTable[bValue]))
				return FALSE;
			dwAccumulator <<= 6; // Prepare next 6 bits
		}
		DWORD dwPaddingBytes = 4 - dwOutputBytes; // Number of padding bytes
		if (! m_EncStream.WriteByte('=', dwPaddingBytes))
			return FALSE;
	}
	return TRUE;
}

/**
 * @return true if data has been written.
 */
BOOL CXmlWriter::WriteStartDocument(void)
{
	_ASSERTE(m_eWriterState == WS_NODATA);
	if (m_eWriterState != WS_NODATA)
		return FALSE;
	m_EncStream.Reset();
	if (! m_EncStream.WriteAscii("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"))
		return FALSE;
	m_eWriterState = WS_DOCUMENT;
	return TRUE;
}

/**
 * @param pszString - text value.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteString(PCTSTR pszString)
{
	_ASSERTE(pszString != NULL);
	if (! FinalizeElement())
		return FALSE;
	_ASSERTE(m_eWriterState == WS_TEXT || m_eWriterState == WS_ATTRUBUTE);
	if (m_eWriterState == WS_TEXT)
		return WriteEscaped(pszString, EF_ESCAPENONE);
	else if (m_eWriterState == WS_ATTRUBUTE)
		return WriteEscaped(pszString, EF_ESCAPEQUOTMARKS);
	else
		return FALSE;
}

/**
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::FinalizeElement(void)
{
	if (m_eWriterState != WS_ELEMENT)
		return TRUE;
	if (! m_EncStream.WriteByte('>'))
		return FALSE;
	m_eWriterState = WS_TEXT;
	return TRUE;
}

/**
 * @param pszName - the name of the DOCTYPE. This must be non-empty.
 * @param pszPubID - if non-null it also writes PUBLIC "pubid" "sysid" where pubid and sysid are replaced with the value of the given arguments.
 * @param pszSysID - If pubid is a null reference and sysid is non-null it writes SYSTEM "sysid" where sysid is replaced with the value of this argument.
 * @param pszSubset - if non-null it writes [subset] where subset is replaced with the value of this argument.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteDocType(PCTSTR pszName, PCTSTR pszPubID, PCTSTR pszSysID, PCTSTR pszSubset)
{
	_ASSERTE(pszName != NULL);
	if (m_eWriterState != WS_DOCUMENT)
		return FALSE;
	if (! m_EncStream.WriteAscii("<!DOCTYPE "))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszName))
		return FALSE;
	if (pszPubID != NULL)
	{
		if (! m_EncStream.WriteAscii(" PUBLIC \""))
			return FALSE;
		if (! m_EncStream.WriteUTF8Bin(pszPubID))
			return FALSE;
		if (! m_EncStream.WriteByte('\"'))
			return FALSE;
		_ASSERTE(pszSysID != NULL);
		if (! m_EncStream.WriteAscii(" \""))
			return FALSE;
		if (! m_EncStream.WriteUTF8Bin(pszSysID))
			return FALSE;
		if (! m_EncStream.WriteByte('\"'))
			return FALSE;
	}
	else if (pszSysID != NULL)
	{
		if (! m_EncStream.WriteAscii(" SYSTEM \""))
			return FALSE;
		if (! m_EncStream.WriteUTF8Bin(pszSysID))
			return FALSE;
		if (! m_EncStream.WriteByte('\"'))
			return FALSE;
	}
	if (pszSubset != NULL)
	{
		if (! m_EncStream.WriteAscii(" [\r\n"))
			return FALSE;
		if (! m_EncStream.WriteUTF8Bin(pszSubset))
			return FALSE;
		if (! m_EncStream.WriteAscii("\r\n]"))
			return FALSE;
	}
	if (! m_EncStream.WriteByte('>'))
		return FALSE;
	return TRUE;
}

/**
 * @param pszLocalName - the local name of the element.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteStartElement(PCTSTR pszLocalName)
{
	_ASSERTE(pszLocalName != NULL);
	if (! FinalizeElement())
		return FALSE;
	_ASSERTE(m_eWriterState == WS_DOCUMENT || m_eWriterState == WS_TEXT);
	if (m_eWriterState != WS_DOCUMENT && m_eWriterState != WS_TEXT)
		return FALSE;
	if (! WriteTagIndent())
		return FALSE;
	if (! m_EncStream.WriteByte('<'))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszLocalName))
		return FALSE;
	m_arrOpenElements.AddItem(pszLocalName);
	m_eWriterState = WS_ELEMENT;
	m_bTopmostTag = TRUE;
	return TRUE;
}

/**
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteEndElement(void)
{
	_ASSERTE(m_eWriterState == WS_ELEMENT || m_eWriterState == WS_TEXT);
	if (m_eWriterState != WS_ELEMENT && m_eWriterState != WS_TEXT)
		return FALSE;
	int nItemIndex = m_arrOpenElements.GetCount();
	_ASSERTE(nItemIndex > 0);
	if (nItemIndex < 1)
		return FALSE;
	--nItemIndex;
	if (m_eWriterState == WS_TEXT)
	{
		CStrHolder strElementName = m_arrOpenElements[nItemIndex];
		m_arrOpenElements.DeleteItem(nItemIndex);
		if (! m_bTopmostTag && ! WriteTagIndent())
			return FALSE;
		if (! m_EncStream.WriteAscii("</"))
			return FALSE;
		if (! m_EncStream.WriteUTF8Bin(strElementName))
			return FALSE;
		if (! m_EncStream.WriteByte('>'))
			return FALSE;
	}
	else
	{
		m_arrOpenElements.DeleteItem(nItemIndex);
		if (! m_EncStream.WriteAscii("/>"))
			return FALSE;
		m_eWriterState = WS_TEXT;
	}
	m_bTopmostTag = FALSE;
	return TRUE;
}

/**
 * @param pszLocalName - the local name of the element.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteStartAttribute(PCTSTR pszLocalName)
{
	_ASSERTE(pszLocalName != NULL);
	_ASSERTE(m_eWriterState == WS_ELEMENT);
	if (m_eWriterState != WS_ELEMENT)
		return FALSE;
	if (! m_EncStream.WriteByte(' '))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszLocalName))
		return FALSE;
	if (! m_EncStream.WriteAscii("=\""))
		return FALSE;
	m_eWriterState = WS_ATTRUBUTE;
	return TRUE;
}

/**
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteEndAttribute(void)
{
	_ASSERTE(m_eWriterState == WS_ATTRUBUTE);
	if (m_eWriterState != WS_ATTRUBUTE)
		return FALSE;
	if (! m_EncStream.WriteByte('\"'))
		return FALSE;
	m_eWriterState = WS_ELEMENT;
	return TRUE;
}

/**
 * @return true if data has been written.
 */
BOOL CXmlWriter::WriteTagIndent(void)
{
	if (m_chIndentChar != '\0')
	{
		if (! WriteNewLine())
			return FALSE;
		if (! WriteIndent())
			return FALSE;
	}
	return TRUE;
}

/**
 * @return true if data has been written.
 */
BOOL CXmlWriter::WriteIndent(void)
{
	_ASSERTE(m_chIndentChar != '\0');
	int iNumOpenElements = m_arrOpenElements.GetCount();
	return (iNumOpenElements > 0 ? m_EncStream.WriteByte(m_chIndentChar, m_dwIndentation * iNumOpenElements) : TRUE);
}

/**
 * @param pszLocalName - the local name of the processing instruction.
 * @param pszString - the value of the processing instruction.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteProcessingInstruction(PCTSTR pszLocalName, PCTSTR pszString)
{
	_ASSERTE(pszLocalName != NULL && pszString != NULL);
	if (! FinalizeElement())
		return FALSE;
	_ASSERTE(m_eWriterState == WS_DOCUMENT || m_eWriterState == WS_TEXT);
	if (m_eWriterState != WS_DOCUMENT && m_eWriterState != WS_TEXT)
		return FALSE;
	m_bTopmostTag = FALSE;
	if (! WriteTagIndent())
		return FALSE;
	if (! m_EncStream.WriteAscii("<?"))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszLocalName))
		return FALSE;
	if (! m_EncStream.WriteByte(' '))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszString))
		return FALSE;
	if (! m_EncStream.WriteAscii("?>"))
		return FALSE;
	return TRUE;
}

/**
 * @param pszString - text value.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteCData(PCTSTR pszString)
{
	_ASSERTE(pszString != NULL);
	if (! FinalizeElement())
		return FALSE;
	_ASSERTE(m_eWriterState == WS_TEXT);
	if (m_eWriterState != WS_TEXT)
		return FALSE;
	m_bTopmostTag = FALSE;
	if (! WriteTagIndent())
		return FALSE;
	if (! m_EncStream.WriteAscii("<![CDATA["))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszString))
		return FALSE;
	if (! m_EncStream.WriteAscii("]]>"))
		return FALSE;
	return TRUE;
}

/**
 * @param pszString - comment text.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteComment(PCTSTR pszString)
{
	_ASSERTE(pszString != NULL);
	if (! FinalizeElement())
		return FALSE;
	_ASSERTE(m_eWriterState == WS_DOCUMENT || m_eWriterState == WS_TEXT);
	if (m_eWriterState != WS_DOCUMENT && m_eWriterState != WS_TEXT)
		return FALSE;
	m_bTopmostTag = FALSE;
	if (! WriteTagIndent())
		return FALSE;
	if (! m_EncStream.WriteAscii("<!--"))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszString))
		return FALSE;
	if (! m_EncStream.WriteAscii("-->"))
		return FALSE;
	return TRUE;
}

/**
 * @param ch - character for which character entity should be generated.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteCharEntity(TCHAR ch)
{
	_ASSERTE(m_eWriterState == WS_TEXT || m_eWriterState == WS_ATTRUBUTE);
	if (m_eWriterState != WS_TEXT && m_eWriterState != WS_ATTRUBUTE)
		return FALSE;
	int nCharSize;
	TCHAR arrChar[2] = { ch, _T('\0') };
	if (! m_EncStream.WriteAscii("&#x"))
		return FALSE;
	if (! m_EncStream.WriteUTF8Hex(arrChar, nCharSize))
		return FALSE;
	if (! m_EncStream.WriteByte(';'))
		return FALSE;
	return TRUE;
}

/**
 * @param pszName - entity name.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteEntityRef(PCTSTR pszName)
{
	_ASSERTE(pszName != NULL);
	_ASSERTE(m_eWriterState == WS_TEXT || m_eWriterState == WS_ATTRUBUTE);
	if (m_eWriterState != WS_TEXT && m_eWriterState != WS_ATTRUBUTE)
		return FALSE;
	if (! m_EncStream.WriteByte('&'))
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszName))
		return FALSE;
	if (! m_EncStream.WriteByte(';'))
		return FALSE;
	return TRUE;
}

/**
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteEndDocument(void)
{
	if (! FinalizeElement())
		return FALSE;
	_ASSERTE(m_eWriterState == WS_TEXT);
	if (m_eWriterState != WS_TEXT)
		return FALSE;
	int nNumElements = m_arrOpenElements.GetCount();
	while (nNumElements > 0)
	{
		if (! WriteEndElement())
			return FALSE;
		--nNumElements;
	}
	m_eWriterState = WS_NODATA;
	return TRUE;
}

/**
 * @param pszString - raw XML expression.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteRaw(PCTSTR pszString)
{
	_ASSERTE(pszString != NULL);
	_ASSERTE(m_eWriterState != WS_NODATA);
	if (m_eWriterState == WS_NODATA)
		return FALSE;
	if (! m_EncStream.WriteUTF8Bin(pszString))
		return FALSE;
	return TRUE;
}

/**
 * @param pszLocalName - the local name of the element.
 * @param pszString - the value of the element.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteElementString(PCTSTR pszLocalName, PCTSTR pszString)
{
	_ASSERTE(pszLocalName != NULL);
	if (! WriteStartElement(pszLocalName))
		return FALSE;
	if (pszString && ! WriteString(pszString))
		return FALSE;
	if (! WriteEndElement())
		return FALSE;
	return TRUE;
}

/**
 * @param pszLocalName - the local name of the attribute.
 * @param pszString - the value of the attribute.
 * @return true if data has been successfully written.
 */
BOOL CXmlWriter::WriteAttributeString(PCTSTR pszLocalName, PCTSTR pszString)
{
	_ASSERTE(pszLocalName != NULL);
	if (! WriteStartAttribute(pszLocalName))
		return FALSE;
	if (pszString && ! WriteString(pszString))
		return FALSE;
	if (! WriteEndAttribute())
		return FALSE;
	return TRUE;
}
