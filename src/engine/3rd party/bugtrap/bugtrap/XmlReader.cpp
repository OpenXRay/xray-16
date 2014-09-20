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

#include "StdAfx.h"
#include "XmlReader.h"
#include "BugTrapUtils.h"
#include "FileStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const TCHAR g_pszXmlInternalError                 [] = _T("Internal error");
static const TCHAR g_pszXmlInputError                    [] = _T("Input error");
static const TCHAR g_pszXmlErrorOutOfMemory              [] = _T("Out of memory");
static const TCHAR g_pszXmlErrorInvalidClosingElement    [] = _T("Invalid closing element");
static const TCHAR g_pszXmlErrorUnexpectedChars          [] = _T("Unexpected characters");
static const TCHAR g_pszXmlErrorUnexpectedToken          [] = _T("Unexpected token");
static const TCHAR g_pszXmlErrorUnexpectedElement        [] = _T("Unexpected element");
static const TCHAR g_pszXmlErrorUnexpectedEndOfElement   [] = _T("Unexpected end of element");
static const TCHAR g_pszXmlErrorUnexpectedEndOfStream    [] = _T("Unexpected end of stream");
static const TCHAR g_pszXmlErrorInvalidNumber            [] = _T("Invalid number");
static const TCHAR g_pszXmlErrorUnuspportedUrlScheme     [] = _T("Unuspported URL scheme");
static const TCHAR g_pszXmlErrorInvalidUrl               [] = _T("Invalid URL");
static const TCHAR g_pszXmlErrorInvalidEntity            [] = _T("Invalid entity");
static const TCHAR g_pszXmlErrorCantOpenExternalResource [] = _T("Can't open external resource");

/// Map of standard entities and their values.
CHash<PCTSTR, TCHAR> CXmlReader::m_mapStdEntities;

/// XML search table.
CXmlReader::CXmlSearchParams CXmlReader::CXmlParser::m_arrInitialXmlSearchParams[] =
{
#define XML_TEXT_PARAM_POS             0
	CXmlSearchParams(_T(""),           &CXmlReader::CXmlParser::XmlTextHandler),
#define XML_ELEMENT_PARAM_POS          1
	CXmlSearchParams(_T("<"),          &CXmlReader::CXmlParser::XmlElementHandler),
#define XML_PROCINSTR_PARAM_POS        2
	CXmlSearchParams(_T("<?"),         &CXmlReader::CXmlParser::ProcInstrHandler),
#define XML_COMMENT_PARAM_POS          3
	CXmlSearchParams(_T("<!--"),       &CXmlReader::CXmlParser::CommentHandler),
#define XML_CDATA_PARAM_POS            4
	CXmlSearchParams(_T("<![CDATA["),  &CXmlReader::CXmlParser::XmlCDataHandler),
#define XML_DOCTYPE_PARAM_POS          5
	CXmlSearchParams(_T("<!DOCTYPE"),  &CXmlReader::CXmlParser::DocTypeHandler)
};

/// DTD search table.
CXmlReader::CXmlSearchParams CXmlReader::CXmlParser::m_arrInitialDtdSearchParams[] =
{
#define DTD_TEXT_PARAM_POS             0
	CXmlSearchParams(_T(""),           &CXmlReader::CXmlParser::DtdTextHandler),
#define DTD_END_PARAM_POS              1
	CXmlSearchParams(_T("]"),          &CXmlReader::CXmlParser::DtdEndHandler),
#define DTD_INVALID_TOKEN_PARAM_POS    2
	CXmlSearchParams(_T("<"),          &CXmlReader::CXmlParser::InvalidTokenHandler),
#define DTD_PROCINSTR_PARAM_POS        3
	CXmlSearchParams(_T("<?"),         &CXmlReader::CXmlParser::ProcInstrHandler),
#define DTD_COND_END_PARAM_POS         4
	CXmlSearchParams(_T("]]>"),        &CXmlReader::CXmlParser::DtdCondtionEndHandler),
#define DTD_COND_PARAM_POS             5
	CXmlSearchParams(_T("<!["),        &CXmlReader::CXmlParser::DtdCondtionHandler),
#define DTD_COMMENT_PARAM_POS          6
	CXmlSearchParams(_T("<!--"),       &CXmlReader::CXmlParser::CommentHandler),
#define DTD_ENTITY_PARAM_POS           7
	CXmlSearchParams(_T("<!ENTITY"),   &CXmlReader::CXmlParser::DtdEntityHandler),
#define DTD_ATTLIST_PARAM_POS          8
	CXmlSearchParams(_T("<!ATTLIST"),  &CXmlReader::CXmlParser::DtdDeclarationHandler),
#define DTD_ELEMENT_PARAM_POS          9
	CXmlSearchParams(_T("<!ELEMENT"),  &CXmlReader::CXmlParser::DtdDeclarationHandler),
#define DTD_NOTATION_PARAM_POS         10
	CXmlSearchParams(_T("<!NOTATION"), &CXmlReader::CXmlParser::DtdDeclarationHandler)
};

/**
 * @param eParserType - parser type.
 * @param rReader - parent XML reader.
 * @param rXmlInputStream - XML input stream.
 */
CXmlReader::CXmlParser::CXmlParser(PARSER_TYPE eParserType, CXmlReader& rReader, CXmlInputStream& rXmlInputStream) :
	m_rReader(rReader), m_rXmlInputStream(rXmlInputStream)
{
	m_pszErrorMessage = NULL;
	if (eParserType == PT_DTD_DOCUMENT)
	{
		m_eParserState = PS_DTD_UNDEFINED;
	}
	else
	{
		_ASSERTE(eParserType == PT_XML_DOCUMENT);
		m_eParserState = PS_XML_UNDEFINED;
	}
	m_nNumEntries = 0;
	m_arrXmlSearchParams = new CXmlSearchParams[countof(m_arrInitialXmlSearchParams)];
	m_arrDtdSearchParams = new CXmlSearchParams[countof(m_arrInitialDtdSearchParams)];
	if (m_arrXmlSearchParams == NULL || m_arrDtdSearchParams == NULL)
	{
		DeleteSearchTables();
		RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
	}
	CopyMemory(m_arrXmlSearchParams, m_arrInitialXmlSearchParams, sizeof(m_arrInitialXmlSearchParams));
	CopyMemory(m_arrDtdSearchParams, m_arrInitialDtdSearchParams, sizeof(m_arrInitialDtdSearchParams));
	m_XmlSearchTable.SetSearchTable(m_arrXmlSearchParams, countof(m_arrInitialXmlSearchParams));
	m_DtdSearchTable.SetSearchTable(m_arrDtdSearchParams, countof(m_arrInitialDtdSearchParams));
}

void CXmlReader::CXmlParser::ApplyContentFilter(void)
{
	ADJACENT_MODE eTextAdjacentMode;
	eTextAdjacentMode = (m_rReader.m_dwContentFilter & XCF_CDATA) != 0 ? AM_TEXT : AM_NONE;
	m_arrXmlSearchParams[XML_TEXT_PARAM_POS].m_dwAdjacentMode = eTextAdjacentMode;
	m_arrXmlSearchParams[XML_CDATA_PARAM_POS].m_dwAdjacentMode = eTextAdjacentMode;
}

CXmlReader::CXmlSearchParams::CXmlSearchParams(void)
{
	m_pszSequence = NULL;
	m_pfnHandler = NULL;
	m_dwAdjacentMode = AM_NONE;
	m_bActive = true;
}

/**
 * @param pszSequence - node sequence.
 * @param pfnHandler - node handler.
 * @param dwAdjacentMode - adjacent flags.
 */
CXmlReader::CXmlSearchParams::CXmlSearchParams(PCTSTR pszSequence, FNodeHandler pfnHandler, DWORD dwAdjacentMode)
{
	m_pszSequence = pszSequence;
	m_pfnHandler = pfnHandler;
	m_dwAdjacentMode = dwAdjacentMode;
	m_bActive = true;
}

/**
 * @param arrSearchParams - array of search parameters.
 * @param nNumParams - number of parameters in the array.
 */
void CXmlReader::CXmlSearchTable::SetSearchTable(CXmlSearchParams arrSearchParams[], int nNumParams)
{
	m_pfnPrefetchedHandler = NULL;
	m_dwAdjacentMode = AM_NONE;
	m_arrSearchParams = arrSearchParams;
	m_nNumParams = nNumParams;
}

void CXmlReader::CXmlNode::Reset(void)
{
	m_eNodeType = XNT_UNDEFINED;
	m_strNodeName.Reset();
	m_strNodeValue.Reset();
	m_mapAttributes.DeleteAll();
}

#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list

CXmlReader::CXmlReader(void) :
	m_XmlParser(CXmlParser::PT_XML_DOCUMENT, *this, m_XmlInputStream)
{
	Init();
}

/**
 * @param pInputStream - input stream.
 */
CXmlReader::CXmlReader(CInputStream* pInputStream) :
	m_XmlParser(CXmlParser::PT_XML_DOCUMENT, *this, m_XmlInputStream)
{
	Init();
	SetInputStream(pInputStream);
}

/**
 * @param pszUrl - URL.
 */
CXmlReader::CXmlReader(PCTSTR pszUrl) :
	m_XmlParser(CXmlParser::PT_XML_DOCUMENT, *this, m_XmlInputStream)
{
	Init();
	Open(pszUrl);
}

#pragma warning(pop)

void CXmlReader::Init(void)
{
	InitStdEntities();
	m_bReleaseInputStream = false;
	SetContentFilter(XCF_ALL);
}

void CXmlReader::ReleaseInputStream(void)
{
	if (m_bReleaseInputStream)
	{
		CInputStream* pInputStream = m_DecStream.GetInputStream();
		m_DecStream.SetInputStream(NULL);
		m_XmlInputStream.SetInputStream(NULL);
		delete pInputStream;
		pInputStream = NULL;
		m_bReleaseInputStream = false;
	}
}

void CXmlReader::CXmlInputStream::InitBackBuffer(void)
{
	m_nBackBufferSize = 32;
	m_nBackBufferReadPos = 0;
	m_nBackBufferWritePos = 0;
	m_pBackBuffer = new TCHAR[m_nBackBufferSize];
	if (m_pBackBuffer == NULL)
	{
		m_nBackBufferSize = 0;
		RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
	}
}

void CXmlReader::InitStdEntities(void)
{
	if (m_mapStdEntities.IsEmpty())
	{
		m_mapStdEntities.SetAt(_T("amp"), _T('&'));
		m_mapStdEntities.SetAt(_T("apos"), _T('\''));
		m_mapStdEntities.SetAt(_T("quot"), _T('\"'));
		m_mapStdEntities.SetAt(_T("lt"), _T('<'));
		m_mapStdEntities.SetAt(_T("gt"), _T('>'));
	}
}

void CXmlReader::Close(void)
{
	ReleaseInputStream();
	m_pszErrorMessage = NULL;
	m_mapXmlEntities.DeleteAll();
	m_mapDtdEntities.DeleteAll();
	m_arrOpenElements.DeleteAll();
}

/**
 * @param pszUrl - URL.
 * @return true if XML document has been located successfully.
 */
bool CXmlReader::Open(PCTSTR pszUrl)
{
	CInputStream* pInputStream = m_XmlParser.CreateInputStream(pszUrl);
	if (pInputStream == NULL)
	{
		m_pszErrorMessage = m_XmlParser.GetErrorMessage();
		return false;
	}
	SetInputStream(pInputStream);
	m_bReleaseInputStream = true;
	m_pszErrorMessage = NULL;
	return true;
}

/**
 * @param pInputStream - input stream.
 */
void CXmlReader::SetInputStream(CInputStream* pInputStream)
{
	Close();
	if (pInputStream != NULL)
	{
		m_DecStream.SetInputStream(pInputStream);
		m_DecStream.CheckEncoding(TXTENC_UTF8);
		m_XmlInputStream.SetInputStream(&m_DecStream);
	}
}


/**
 * @param pInputStream - input stream.
 */
void CXmlReader::CXmlInputStream::SetInputStream(CCharInputStream* pInputStream)
{
	m_pInputStream = pInputStream;
	m_nBackBufferReadPos = 0;
	m_nBackBufferWritePos = 0;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param rSearchTable - search table.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ProcessNode(CXmlNode& rXmlNode, CXmlSearchTable& rSearchTable, PVOID pParam)
{
	if (rSearchTable.m_pfnPrefetchedHandler != NULL)
	{
		rXmlNode.Reset();
		FNodeHandler pfnHandler = rSearchTable.m_pfnPrefetchedHandler;
		rSearchTable.m_pfnPrefetchedHandler = NULL;
		return (this->*pfnHandler)(rXmlNode, pParam);
	}
	CXmlSearchParams* arrSearchParams = rSearchTable.m_arrSearchParams;
	int nNumParams = rSearchTable.m_nNumParams;
	for (int nParamNum = 0; nParamNum < nNumParams; ++nParamNum)
		arrSearchParams[nParamNum].m_bActive = true;
	int nSequencePos = 0,
		nDefaultParamNum = -1,
		nDefaultSequenceLength = 0,
		nLongestParamNum = -1,
		nLongestSequenceLength = 0;
	for (;;)
	{
		TCHAR arrChar[2];
		int nCharSize = ReadChar(arrChar, false);
		if (nCharSize < 0)
			return XMLR_ERROR;
		else if (nCharSize == 0)
		{
			if (nSequencePos == 0 && IsEndOfDocument())
				return XMLR_EOF;
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
			return XMLR_ERROR;
		}
		bool bNotEmptyCriteria = false;
		for (int nParamNum = 0; nParamNum < nNumParams; ++nParamNum)
		{
			CXmlSearchParams* pSearchParams = arrSearchParams + nParamNum;
			if (pSearchParams->m_bActive)
			{
				if (pSearchParams->m_pszSequence[nSequencePos] == *arrChar)
				{
					_ASSERTE(nCharSize == 1);
					bNotEmptyCriteria = true;
					nLongestParamNum = nParamNum;
					nLongestSequenceLength = nSequencePos;
				}
				else
				{
					pSearchParams->m_bActive = false;
					if (pSearchParams->m_pszSequence[nSequencePos] == _T('\0'))
					{
						nDefaultParamNum = nParamNum;
						nDefaultSequenceLength = nSequencePos;
					}
				}
			}
		}
		if (! bNotEmptyCriteria)
		{
			if (nLongestParamNum >= 0)
			{
				int nRestoreLength = 1 + nLongestSequenceLength - nDefaultSequenceLength;
				CXmlSearchParams* pSearchParams = arrSearchParams + nLongestParamNum;
				PrepareBackBuffer(nRestoreLength + nCharSize);
				UnsafePutCharBack(arrChar, nCharSize);
				UnsafePutCharsBack(pSearchParams->m_pszSequence + nDefaultSequenceLength, nRestoreLength);
			}
			else
				PutCharBack(arrChar, nCharSize);
			if (nDefaultParamNum >= 0)
			{
				CXmlSearchParams* pSearchParams = arrSearchParams + nDefaultParamNum;
				FNodeHandler pfnHandler = pSearchParams->m_pfnHandler;
				_ASSERTE(pfnHandler != NULL);
				DWORD dwAdjacentMode = pSearchParams->m_dwAdjacentMode;
				if (rSearchTable.m_dwAdjacentMode == AM_NONE)
				{
					rXmlNode.Reset();
					rSearchTable.m_dwAdjacentMode = dwAdjacentMode;
					return (this->*pfnHandler)(rXmlNode, pParam);
				}
				else
				{
					if ((rSearchTable.m_dwAdjacentMode & dwAdjacentMode) != 0)
					{
						return (this->*pfnHandler)(rXmlNode, pParam);
					}
					else
					{
						rSearchTable.m_pfnPrefetchedHandler = pfnHandler;
						rSearchTable.m_dwAdjacentMode = dwAdjacentMode;
						return XMLR_EOS;
					}
				}
			}
		}
		else
		{
			++nSequencePos;
			continue;
		}
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
}

/**
 * @param arrChar - character data.
 * @return number of characters in one symbol.
 */
int CXmlReader::CXmlParser::ReadNonSpaceChar(TCHAR arrChar[2])
{
	for (;;)
	{
		int nCharSize = ReadChar(arrChar, true);
		if (nCharSize > 0)
		{
			_ASSERTE(nCharSize < 2);
			WORD arrCharType[2];
#ifdef _UNICODE
			GetStringTypeW(CT_CTYPE1, arrChar, nCharSize, arrCharType);
#else
			GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, arrChar, nCharSize, arrCharType);
#endif
			if (*arrCharType & C1_SPACE)
				continue;
		}
		return nCharSize;
	}
}

/**
 * @param bRequired - true if space characters are required.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::SkipSpaces(bool bRequired)
{
	bool bEmpty = true;
	for (;;)
	{
		TCHAR arrChar[2];
		int nCharSize = ReadChar(arrChar, true);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		_ASSERTE(nCharSize < 2);
		WORD arrCharType[2];
#ifdef _UNICODE
		GetStringTypeW(CT_CTYPE1, arrChar, nCharSize, arrCharType);
#else
		GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, arrChar, nCharSize, arrCharType);
#endif
		if ((*arrCharType & C1_SPACE) == 0)
		{
			if (bEmpty && bRequired)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
				return XMLR_ERROR;
			}
			else
			{
				PutCharBack(arrChar, nCharSize);
				return XMLR_CONTINUE;
			}
		}
		bEmpty = false;
	}
}

/**
 * @param chValue - output character value.
 * @return true if buffer is not empty.
 */
bool CXmlReader::CXmlInputStream::ReadCharFromBackBuffer(TCHAR& chValue)
{
	if (IsBackBufferEmpty())
		return false;
	chValue = m_pBackBuffer[m_nBackBufferReadPos];
	m_nBackBufferReadPos = (m_nBackBufferReadPos + 1) % m_nBackBufferSize;
	return true;
}

/**
 * @param arrChar - character data.
 * @return number of characters in one symbol.
 */
int CXmlReader::CXmlInputStream::ReadChar(TCHAR arrChar[2])
{
	int nCharSize;
	if (ReadCharFromBackBuffer(arrChar[0]))
	{
#ifdef _UNICODE
		if (IsUnicodeLeadChar((const BYTE*)arrChar))
#else
		if (IsDBCSLeadByte(*arrChar))
#endif
		{
			if (! ReadCharFromBackBuffer(arrChar[1]))
			{
				_ASSERT(FALSE);
				nCharSize = -1;
			}
			else
				nCharSize = 2;
		}
		else
			nCharSize = 1;
	}
	else
	{
		_ASSERTE(m_pInputStream != NULL);
		nCharSize = m_pInputStream->ReadChar(arrChar);
	}
	return nCharSize;
}

/**
 * @param arrChar - character data.
 * @param bRequired - true if character is required.
 * @return number of characters in one symbol.
 */
int CXmlReader::CXmlParser::ReadChar(TCHAR arrChar[2], bool bRequired)
{
	int nCharSize = ReadChar(arrChar);
	if (nCharSize < 0)
	{
		m_pszErrorMessage = g_pszXmlInputError;
	}
	else if (nCharSize == 0 && bRequired)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
		nCharSize = -1;
	}
	return nCharSize;
}

/**
 * Specifies number of characters to reserve space for.
 * @param nNumChars - number of characters to reserve space for.
 */
void CXmlReader::CXmlInputStream::PrepareBackBuffer(int nNumChars)
{
	int nCharsUsed = GetBackBufferLength();
	if (nCharsUsed + nNumChars >= m_nBackBufferSize)
	{
		int nNewBufferSize = (m_nBackBufferSize + nNumChars) * 2;
		PTCHAR pBackBuffer = new TCHAR[nNewBufferSize];
		if (pBackBuffer == NULL)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		CopyMemory(pBackBuffer, m_pBackBuffer + m_nBackBufferReadPos, nCharsUsed * sizeof(TCHAR));
		delete[] m_pBackBuffer;
		m_pBackBuffer = pBackBuffer;
		m_nBackBufferSize = nNewBufferSize;
		m_nBackBufferReadPos = 0;
		m_nBackBufferWritePos = nCharsUsed;
	}
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::XmlTextHandler(CXmlNode& rXmlNode, PVOID /*pParam*/)
{
	_ASSERTE(
		rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED ||
		rXmlNode.m_eNodeType == CXmlNode::XNT_TEXT);
	if (m_eParserState == PS_XML_DOCUMENT)
		rXmlNode.m_eNodeType = CXmlNode::XNT_TEXT;
	return ReadText(rXmlNode.m_strNodeValue, _T("<"), false);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::InvalidTokenHandler(CXmlNode& /*rXmlNode*/, PVOID /*pParam*/)
{
	m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
	return XMLR_ERROR;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DtdTextHandler(CXmlNode& rXmlNode, PVOID /*pParam*/)
{
	_ASSERTE(rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED);
	return ReadText(rXmlNode.m_strNodeValue, _T("<]"), false);
}

/**
 * @return entity character.
 */
TCHAR CXmlReader::CXmlParser::GetEntityChar(void) const
{
	switch (m_eParserState)
	{
	case PS_XML_DOCUMENT:
		return _T('&');
	case PS_DTD_DOCUMENT:
	case PS_EMBEDDED_DTD:
		return _T('%');
	default:
		return _T('\0');
	}
}

/**
 * @return entities map.
 */
const CHash<CStrStream, CStrStream>* CXmlReader::CXmlParser::GetEntities(void) const
{
	switch (m_eParserState)
	{
	case PS_XML_DOCUMENT:
		return &m_rReader.m_mapXmlEntities;
	case PS_DTD_DOCUMENT:
	case PS_EMBEDDED_DTD:
		return &m_rReader.m_mapDtdEntities;
	default:
		return NULL;
	}
}

/**
 * @return entities map.
 */
CHash<CStrStream, CStrStream>* CXmlReader::CXmlParser::GetEntities(void)
{
	switch (m_eParserState)
	{
	case PS_XML_DOCUMENT:
		return &m_rReader.m_mapXmlEntities;
	case PS_DTD_DOCUMENT:
	case PS_EMBEDDED_DTD:
		return &m_rReader.m_mapDtdEntities;
	default:
		return NULL;
	}
}

/**
 * @param pOutputStream - output stream.
 * @param pszTerminators - list of text terminators.
 * @param bInAttribute - true when reading attribute value.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadTextEx(CStrStream* pOutputStream, PCTSTR pszTerminators, bool bInAttribute)
{
	bool bEndOfDocument, bIgnoreText;
	if (! bInAttribute)
	{
		if (m_eParserState == PS_XML_UNDEFINED)
			m_eParserState = PS_XML_PROLOGUE;
		else if (m_eParserState == PS_DTD_UNDEFINED)
			m_eParserState = PS_DTD_DOCUMENT;
		bEndOfDocument = IsEndOfDocument();
		bIgnoreText = m_eParserState != PS_XML_DOCUMENT;
	}
	else
	{
		bEndOfDocument = false;
		bIgnoreText = false;
	}
	TCHAR chLastLn = _T('\0');
	TCHAR chEntity = GetEntityChar();
	int nSkipChars = 0;
	XML_RESULT eResult;
	for (;;)
	{
		TCHAR arrChar[2];
		WORD arrCharType[2];
		int nCharSize = ReadChar(arrChar, false);
		if (nCharSize == 0)
		{
			if (! bEndOfDocument)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
				return XMLR_ERROR;
			}
			return XMLR_EOF;
		}
		if (nCharSize < 0)
			return XMLR_ERROR;
		if (nSkipChars <= 0)
		{
			if (_tcschr(pszTerminators, *arrChar))
			{
				PutCharBack(arrChar, nCharSize);
				return ((m_rReader.m_dwContentFilter & XCF_CDATA) != 0 ? XMLR_CONTINUE : XMLR_EOS);
			}
			else if (*arrChar == chEntity)
			{
				eResult = ReadEntity(nSkipChars);
				if (eResult != XMLR_CONTINUE)
					return XMLR_ERROR;
				continue;
			}
		}
		else
			--nSkipChars;
#ifdef _UNICODE
		GetStringTypeW(CT_CTYPE1, arrChar, nCharSize, arrCharType);
#else
		GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, arrChar, nCharSize, arrCharType);
#endif
		if (*arrCharType & C1_SPACE)
		{
			if (pOutputStream != NULL && ! bIgnoreText)
			{
				if (*arrChar == _T('\r') || *arrChar == _T('\n'))
				{
					_ASSERTE(nCharSize == 1);
					bool bSkipNextChar = chLastLn && *arrChar != chLastLn;
					chLastLn = *arrChar;
					if (bSkipNextChar)
						continue;
					else
						*arrChar = _T('\n');
				}
				else
					chLastLn = _T('\0');
				PutCharToStream(*pOutputStream, arrChar, nCharSize);
			}
		}
		else
		{
			if (bIgnoreText)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
				return XMLR_ERROR;
			}
			if (pOutputStream != NULL)
			{
				chLastLn = _T('\0');
				PutCharToStream(*pOutputStream, arrChar, nCharSize);
			}
		}
	}
}

/**
 * @param nValueLength - value length.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNumericEntity(int& nValueLength)
{
	nValueLength = 0;
	TCHAR arrChar[2];
	int nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	XML_RESULT eResult;
	DWORD dwUnicodeChar = 0;
	if (*arrChar == _T('x') || *arrChar == _T('X'))
	{
		eResult = ReadHexNumber(dwUnicodeChar);
	}
	else
	{
		PutCharBack(arrChar, nCharSize);
		eResult = ReadDecNumber(dwUnicodeChar);
	}
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar != _T(';'))
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
	if (dwUnicodeChar == 0)
	{
		m_pszErrorMessage = g_pszXmlErrorInvalidNumber;
		return XMLR_ERROR;
	}
	WCHAR arrCharW[2];
	if (dwUnicodeChar >= 0x10000)
	{
		dwUnicodeChar -= 0x10000;
		arrCharW[0] = (WCHAR)(HIGH_SURROGATE_START + ((dwUnicodeChar >> 10) & (HIGH_SURROGATE_END - HIGH_SURROGATE_START)));
		arrCharW[1] = (WCHAR)(LOW_SURROGATE_START + (dwUnicodeChar & (LOW_SURROGATE_END - LOW_SURROGATE_START)));
		nCharSize = 2;
	}
	else
	{
		arrCharW[0] = (WCHAR)dwUnicodeChar;
		nCharSize = 1;
	}
#ifdef _UNICODE
	PutCharBack(arrCharW, nCharSize);
#else
	CHAR arrCharA[2];
	nCharSize = WideCharToMultiByte(CP_ACP, 0, arrCharW, nCharSize, arrCharA, countof(arrCharA), NULL, NULL);
	PutCharBack(arrCharA, nCharSize);
#endif
	nValueLength += nCharSize;
	return XMLR_CONTINUE;
}

/**
 * @param nValueLength - value length.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNamedEntity(int& nValueLength)
{
	nValueLength = 0;
	CStrStream strEntityName(32);
	XML_RESULT eResult = ReadName(strEntityName);
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	TCHAR arrChar[2];
	int nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar != _T(';'))
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
	TCHAR chValue;
	CHash<CStrStream, CStrStream>* pMapEntities = GetEntities();
	_ASSERTE(pMapEntities != NULL);
	const CStrStream* pstrEntityValue = pMapEntities != NULL ? pMapEntities->Lookup(strEntityName) : NULL;
	if (pstrEntityValue != NULL)
	{
		PutCharsBack(*pstrEntityValue);
	}
	else if (m_mapStdEntities.Lookup(strEntityName, chValue))
	{
		PutCharBack(chValue);
		nValueLength = 1;
	}
	else
	{
		m_pszErrorMessage = g_pszXmlErrorInvalidEntity;
		return XMLR_ERROR;
	}
	return XMLR_CONTINUE;
}

/**
 * @param nValueLength - value length.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadEntity(int& nValueLength)
{
	nValueLength = 0;
	TCHAR arrChar[2];
	int nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar == _T('#'))
	{
		return ReadNumericEntity(nValueLength);
	}
	else
	{
		PutCharBack(arrChar, nCharSize);
		return ReadNamedEntity(nValueLength);
	}
}

/**
 * @param dwNumber - numeric value.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadDecNumber(DWORD& dwNumber)
{
	dwNumber = 0;
	for (;;)
	{
		TCHAR arrChar[2];
		int nCharSize = ReadChar(arrChar, true);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		if (*arrChar >= _T('0') && *arrChar <= _T('9'))
		{
			dwNumber *= 10;
			dwNumber += *arrChar - _T('0');
		}
		else
		{
			PutCharBack(arrChar, nCharSize);
			return XMLR_CONTINUE;
		}
	}
}

/**
 * @param dwNumber - numeric value.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadHexNumber(DWORD& dwNumber)
{
	dwNumber = 0;
	for (;;)
	{
		TCHAR arrChar[2];
		int nCharSize = ReadChar(arrChar, true);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		if (*arrChar >= _T('0') && *arrChar <= _T('9'))
		{
			dwNumber *= 16;
			dwNumber += *arrChar - _T('0');
		}
		else if (*arrChar >= _T('A') && *arrChar <= _T('Z'))
		{
			dwNumber *= 16;
			dwNumber += *arrChar - _T('A') + 10;
		}
		else if (*arrChar >= _T('a') && *arrChar <= _T('z'))
		{
			dwNumber *= 16;
			dwNumber += *arrChar - _T('a') + 10;
		}
		else
		{
			PutCharBack(arrChar, nCharSize);
			return XMLR_CONTINUE;
		}
	}
}

/**
 * @param pOutputStream - pointer to the output stream or @a NULL.
 * @param pszTerminator - terminator sequence.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadUntilEx(CStrStream* pOutputStream, PCTSTR pszTerminator)
{
	PCTSTR pszTemp = pszTerminator;
	while (*pszTemp)
	{
		TCHAR arrChar[2];
		int nCharSize = ReadChar(arrChar, true);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		_ASSERTE(nCharSize <= 2);
		if (arrChar[0] == pszTemp[0] &&
			(nCharSize == 1 || arrChar[1] == pszTemp[1]))
		{
			pszTemp += nCharSize;
			continue;
		}
		if (pOutputStream != NULL)
		{
			int nNumChars = pszTemp - pszTerminator;
			PutCharsToStream(*pOutputStream, pszTerminator, nNumChars);
			PutCharToStream(*pOutputStream, arrChar, nCharSize);
		}
		pszTemp = pszTerminator;
	}
	return XMLR_CONTINUE;
}

/**
 * @param pOutputStream - output stream.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNameEx(CStrStream* pOutputStream)
{
	int nCharPos = 0;
	for (;;)
	{
		TCHAR arrChar[2];
		int nCharSize = ReadChar(arrChar, true);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		_ASSERTE(nCharSize <= 2);
		WORD arrCharType[2];
#ifdef _UNICODE
		GetStringTypeW(CT_CTYPE1, arrChar, nCharSize, arrCharType);
#else
		GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, arrChar, nCharSize, arrCharType);
#endif
		if (nCharPos == 0 ?
				(*arrCharType & C1_ALPHA) || *arrChar == _T('_') || *arrChar == _T(':') :
				(*arrCharType & (C1_ALPHA | C1_DIGIT)) || *arrChar == _T('.') || *arrChar == _T('-') || *arrChar == _T('_') || *arrChar == _T(':')
			)
		{
			if (pOutputStream != NULL)
				PutCharToStream(*pOutputStream, arrChar, nCharSize);
		}
		else if (nCharPos > 0)
		{
			PutCharBack(arrChar, nCharSize);
			return XMLR_CONTINUE;
		}
		else
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
			return XMLR_ERROR;
		}
		++nCharPos;
	}
}

/**
 * @param pszName - attribute name.
 * @return attribute value.
 */
PCTSTR CXmlReader::CAttributesList::CaselessGetAt(PCTSTR pszName) const
{
	POSITION posAttr = GetStartPosition();
	while (posAttr != NULL)
	{
		if (_tcsicmp(GetNameAt(posAttr), pszName) == 0)
			return GetValueAt(posAttr);
		posAttr = GetNextPosition(posAttr);
	}
	return NULL;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param eTagType - type of tag.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::TagHandler(CXmlNode& rXmlNode, TAG_TYPE eTagType)
{
	_ASSERTE(rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED);
	XML_RESULT eResult;
	TCHAR arrChar[2];
	int nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar == _T('/'))
	{
		if (m_eParserState != PS_XML_DOCUMENT)
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
			return XMLR_ERROR;
		}
		if (eTagType != TT_ELEMENT)
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
			return XMLR_ERROR;
		}
		rXmlNode.m_eNodeType = CXmlNode::XNT_ELEMENT_END;
	}
	else
	{
		switch (eTagType)
		{
		case TT_ELEMENT:
			if (m_eParserState != PS_XML_UNDEFINED &&
				m_eParserState != PS_XML_PROLOGUE &&
				m_eParserState != PS_XML_DOCUMENT)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
				return XMLR_ERROR;
			}
			rXmlNode.m_eNodeType = CXmlNode::XNT_ELEMENT_BEGIN;
			break;
		case TT_PROCINSTR:
			if (m_eParserState != PS_XML_UNDEFINED &&
				m_eParserState != PS_XML_PROLOGUE &&
				m_eParserState != PS_XML_DOCUMENT &&
				m_eParserState != PS_XML_EPILOGUE &&
				m_eParserState != PS_DTD_UNDEFINED &&
				m_eParserState != PS_DTD_DOCUMENT &&
				m_eParserState != PS_EMBEDDED_DTD)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
				return XMLR_ERROR;
			}
			rXmlNode.m_eNodeType = CXmlNode::XNT_PROCINSTR;
			break;
		default:
			_ASSERT(FALSE);
			m_pszErrorMessage = g_pszXmlInternalError;
			return XMLR_ERROR;
		}
		PutCharBack(arrChar, nCharSize);
	}
	eResult = ReadName(rXmlNode.m_strNodeName);
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	for (;;)
	{
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		CArray<CStrHolder>& arrOpenElements = m_rReader.m_arrOpenElements;
		switch (eTagType)
		{
		case TT_ELEMENT:
			if (*arrChar == _T('/'))
			{
				if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
				{
					m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
					return XMLR_ERROR;
				}
				rXmlNode.m_eNodeType = CXmlNode::XNT_ELEMENT;
				nCharSize = ReadChar(arrChar, true);
				if (nCharSize <= 0)
					return XMLR_ERROR;
				if (*arrChar != _T('>'))
				{
					m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
					return XMLR_ERROR;
				}
				if (arrOpenElements.IsEmpty())
					m_eParserState = PS_XML_EPILOGUE;
				else
					m_eParserState = PS_XML_DOCUMENT;
				return XMLR_EOS;
			}
			else if (*arrChar == _T('>'))
			{
				if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
				{
					int nLastElement = arrOpenElements.GetCount() - 1;
					if (nLastElement < 0 ||
						_tcscmp(arrOpenElements.GetAt(nLastElement), rXmlNode.m_strNodeName) != 0)
					{
						m_pszErrorMessage = g_pszXmlErrorInvalidClosingElement;
						return XMLR_ERROR;
					}
					arrOpenElements.DeleteItem(nLastElement);
					_ASSERTE(m_eParserState == PS_XML_DOCUMENT);
					if (nLastElement == 0)
						m_eParserState = PS_XML_EPILOGUE;
				}
				else
				{
					arrOpenElements.AddItem(rXmlNode.m_strNodeName);
					m_eParserState = PS_XML_DOCUMENT;
				}
				return XMLR_EOS;
			}
			break;
		case TT_PROCINSTR:
			if (*arrChar == _T('?'))
			{
				nCharSize = ReadChar(arrChar, true);
				if (nCharSize <= 0)
					return XMLR_ERROR;
				if (*arrChar != _T('>'))
				{
					m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
					return XMLR_ERROR;
				}
				if (_tcsicmp(rXmlNode.m_strNodeName, _T("xml")) == 0)
				{
					if (m_eParserState == PS_XML_UNDEFINED)
					{
						m_eParserState = PS_XML_PROLOGUE;
					}
					else if (m_eParserState == PS_DTD_UNDEFINED)
					{
						m_eParserState = PS_DTD_DOCUMENT;
					}
					else
					{
						m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
						return XMLR_ERROR;
					}
					CAttributesList& rAttributes = rXmlNode.m_mapAttributes;
					PCTSTR pszEncoding = rAttributes.CaselessGetAt(_T("encoding"));
					CBaseDecoder* pDecoder = pszEncoding != NULL ? CBaseDecoder::GetDecoder(pszEncoding) : &CUTF8Decoder::GetInstance();
					m_rXmlInputStream.SetDecoder(pDecoder);
				}
				if ((m_rReader.m_dwContentFilter & XCF_PROCINSTR) != 0)
				{
					rXmlNode.Reset();
					return XMLR_CONTINUE;
				}
				else
					return XMLR_EOS;
			}
			break;
		default:
			_ASSERT(FALSE);
			m_pszErrorMessage = g_pszXmlInternalError;
			return XMLR_ERROR;
		}
		if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
			return XMLR_ERROR;
		}
		PutCharBack(arrChar, nCharSize);
		CStrStream strAtrrName(32);
		eResult = ReadName(strAtrrName);
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		if (*arrChar != _T('='))
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
			return XMLR_ERROR;
		}
		CAttributesList::CAttributesListBase& rAttributes = rXmlNode.m_mapAttributes;
		CStrStream& rAttrValue = rAttributes.GetAt(strAtrrName);
		eResult = ReadString(rAttrValue);
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
	}
}

/**
 * @param pOutputStream - output stream.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadStringEx(CStrStream* pOutputStream)
{
	TCHAR arrChar[2];
	int nCharSize = ReadNonSpaceChar(arrChar);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	TCHAR szQuote[2] = { *arrChar, _T('\0') };
	if (*szQuote != _T('\"') && *szQuote != _T('\''))
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
	XML_RESULT eResult = ReadTextEx(pOutputStream, szQuote, true);
	if (eResult == XMLR_EOF)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
		return XMLR_ERROR;
	}
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	_ASSERTE(*arrChar == *szQuote);
	if (*arrChar != *szQuote)
	{
		m_pszErrorMessage = g_pszXmlInternalError;
		return XMLR_ERROR;
	}
	return XMLR_CONTINUE;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::CommentHandler(CXmlNode& rXmlNode, PVOID /*pParam*/)
{
	if (m_eParserState == PS_XML_UNDEFINED)
		m_eParserState = PS_XML_PROLOGUE;
	else if (m_eParserState == PS_DTD_UNDEFINED)
		m_eParserState = PS_DTD_DOCUMENT;
	_ASSERTE(rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED);
	CStrStream* pOutputStream;
	bool bSkipComments = (m_rReader.m_dwContentFilter & XCF_COMMENT) != 0;
	if (! bSkipComments)
	{
		rXmlNode.m_eNodeType = CXmlNode::XNT_COMMENT;
		pOutputStream = &rXmlNode.m_strNodeValue;
	}
	else
		pOutputStream = NULL;
	XML_RESULT eResult = ReadUntilEx(pOutputStream, _T("--"));
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	TCHAR arrChar[2];
	int nCharSize = ReadChar(arrChar, true);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar != _T('>'))
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
	return (bSkipComments ? XMLR_CONTINUE : XMLR_EOS);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::XmlCDataHandler(CXmlNode& rXmlNode, PVOID /*pParam*/)
{
	if (m_eParserState != PS_XML_DOCUMENT)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	XML_RESULT eResult;
	if ((m_rReader.m_dwContentFilter & XCF_CDATA) != 0)
	{
		_ASSERTE(
			rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED ||
			rXmlNode.m_eNodeType == CXmlNode::XNT_TEXT);
		rXmlNode.m_eNodeType = CXmlNode::XNT_TEXT;
		eResult = ReadUntil(rXmlNode.m_strNodeValue, _T("]]>"));
		return (eResult == XMLR_CONTINUE ? XMLR_CONTINUE : XMLR_ERROR);
	}
	else
	{
		_ASSERTE(rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED);
		rXmlNode.m_eNodeType = CXmlNode::XNT_CDATA;
		eResult = ReadUntil(rXmlNode.m_strNodeValue, _T("]]>"));
		return (eResult == XMLR_CONTINUE ? XMLR_EOS : XMLR_ERROR);
	}
}

/**
 * @param pszUrl - URL.
 * @return input stream object or NULL pointer.
 */
CInputStream* CXmlReader::CXmlParser::CreateInputStream(PCTSTR pszUrl)
{
	TCHAR szPath[MAX_PATH];
	if (PathIsURL(pszUrl))
	{
		if (UrlIsFileUrl(pszUrl))
		{
			DWORD dwPathLength = countof(szPath);
			if (SUCCEEDED(PathCreateFromUrl(pszUrl, szPath, &dwPathLength, 0)))
			{
				pszUrl = szPath;
			}
			else
			{
				m_pszErrorMessage = g_pszXmlErrorInvalidUrl;
				return NULL;
			}
		}
		else
		{
			// at this moment only file URLs are supported
			m_pszErrorMessage = g_pszXmlErrorUnuspportedUrlScheme;
			return NULL;
		}
	}
	TCHAR szFileName[MAX_PATH];
	if (PathIsRelative(pszUrl) &&
		m_rXmlInputStream.GetName(szFileName, countof(szFileName)))
	{
		PathRemoveFileSpec(szFileName);
		PathAppend(szFileName, pszUrl);
		pszUrl = szFileName;
	}
	CFileStream* pFileStream = new CFileStream(0);
	if (pFileStream == NULL)
	{
		m_pszErrorMessage = g_pszXmlErrorOutOfMemory;
		return NULL;
	}
	if (! pFileStream->Open(pszUrl, OPEN_EXISTING, GENERIC_READ, FILE_SHARE_READ))
	{
		m_pszErrorMessage = g_pszXmlErrorCantOpenExternalResource;
		delete pFileStream;
		return NULL;
	}
	return pFileStream;
}

/**
 * @param rExternalName - eternal ID.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadExternalName(CStrStream& rExternalName)
{
	CStrStream strNameType(16);
	XML_RESULT eResult = ReadName(strNameType);
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	if (_tcscmp(strNameType, _T("PUBLIC")) == 0)
	{
		eResult = SkipString();
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
	}
	else if (_tcscmp(strNameType, _T("SYSTEM")) != 0)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	return ReadString(rExternalName);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DocTypeHandler(CXmlNode& /*rXmlNode*/, PVOID /*pParam*/)
{
	if (m_eParserState != PS_XML_UNDEFINED && m_eParserState != PS_XML_PROLOGUE)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	m_eParserState = PS_XML_PROLOGUE;
	XML_RESULT eResult = SkipSpaces(true);
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	eResult = SkipName();
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	TCHAR arrChar[2];
	int nCharSize = ReadNonSpaceChar(arrChar);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar != _T('>') && *arrChar != _T('['))
	{
		PutCharBack(arrChar, nCharSize);
		CStrStream strDtdName(MAX_PATH);
		eResult = ReadExternalName(strDtdName);
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
		CInputStream* pInputStream = CreateInputStream(strDtdName);
		if (pInputStream == NULL)
			return XMLR_ERROR;
		CDecInputStream decInputStream(pInputStream);
		CXmlInputStream xmlInputSteam(&decInputStream);
		xmlInputSteam.CheckEncoding(TXTENC_UTF8);
		CXmlParser xmlParser(CXmlParser::PT_DTD_DOCUMENT, m_rReader, xmlInputSteam);
		eResult = xmlParser.ReadDtdNodes();
		decInputStream.SetInputStream(NULL);
		xmlInputSteam.SetInputStream(NULL);
		delete pInputStream;
		pInputStream = NULL;
		_ASSERTE(eResult != XMLR_CONTINUE && eResult != XMLR_EOS);
		if (eResult == XMLR_CONTINUE || eResult == XMLR_EOS)
		{
			m_pszErrorMessage = g_pszXmlInternalError;
			return XMLR_ERROR;
		}
		if (eResult != XMLR_EOF)
		{
			m_pszErrorMessage = xmlParser.m_pszErrorMessage;
			return XMLR_ERROR;
		}
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
	}
	if (*arrChar == _T('['))
	{
		eResult = ReadDtdNodes();
		_ASSERTE(eResult != XMLR_CONTINUE && eResult != XMLR_EOS);
		if (eResult == XMLR_CONTINUE || eResult == XMLR_EOS)
		{
			m_pszErrorMessage = g_pszXmlInternalError;
			return XMLR_ERROR;
		}
		if (eResult != XMLR_EOF)
			return XMLR_ERROR;
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
	}
	if (*arrChar != _T('>'))
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
	m_eParserState = PS_XML_DOCUMENT;
	return XMLR_CONTINUE;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DtdEntityHandler(CXmlNode& /*rXmlNode*/, PVOID pParam)
{
	if (m_eParserState == PS_DTD_UNDEFINED)
	{
		m_eParserState = PS_DTD_DOCUMENT;
	}
	else if (m_eParserState != PS_DTD_DOCUMENT && m_eParserState != PS_EMBEDDED_DTD)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	bool bProcessingEnabled = pParam != NULL;
	XML_RESULT eResult = SkipSpaces(true);
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	TCHAR arrChar[2];
	int nCharSize = ReadNonSpaceChar(arrChar);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	bool bDtdEntity;
	if (*arrChar == _T('%'))
	{
		bDtdEntity = true;
		eResult = SkipSpaces(true);
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
	}
	else
	{
		bDtdEntity = false;
		PutCharBack(arrChar, nCharSize);
	}
	CStrStream strEntityName(32);
	eResult = ReadName(strEntityName);
	if (eResult != XMLR_CONTINUE)
		return XMLR_ERROR;
	nCharSize = ReadNonSpaceChar(arrChar);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	PutCharBack(arrChar, nCharSize);
	if (*arrChar == _T('\"') || *arrChar == _T('\''))
	{
		// internal entity
		CStrStream strEntityValue(128);
		eResult = ReadString(strEntityValue);
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
		if (bProcessingEnabled)
		{
			CHash<CStrStream, CStrStream>& rMapEntities = bDtdEntity ? m_rReader.m_mapDtdEntities : m_rReader.m_mapXmlEntities;
			rMapEntities.SetAt(strEntityName, strEntityValue);
		}
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		if (*arrChar != _T('>'))
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
			return XMLR_ERROR;
		}
		return XMLR_CONTINUE;
	}
	else
	{
		// external entity
		CStrStream strEntityPath(MAX_PATH);
		eResult = ReadExternalName(strEntityPath);
		if (eResult != XMLR_CONTINUE)
			return XMLR_ERROR;
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		if (*arrChar != _T('>'))
		{
			PutCharBack(arrChar, nCharSize);
			CStrStream strNData(16);
			eResult = ReadName(strNData);
			if (eResult != XMLR_CONTINUE)
				return XMLR_ERROR;
			if (_tcscmp(strNData, _T("NDATA")) != 0)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
				return XMLR_ERROR;
			}
			eResult = SkipName();
			if (eResult != XMLR_CONTINUE)
				return XMLR_ERROR;
			nCharSize = ReadNonSpaceChar(arrChar);
			if (nCharSize <= 0)
				return XMLR_ERROR;
			if (*arrChar != _T('>'))
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
				return XMLR_ERROR;
			}
		}
		else if (bProcessingEnabled)
		{
			CInputStream* pInputStream = CreateInputStream(strEntityName);
			if (pInputStream == NULL)
				return XMLR_ERROR;
			CDecInputStream decInputStream(pInputStream);
			decInputStream.CheckEncoding(TXTENC_UTF8);
			CStrStream strEntityValue(128);
			for (;;)
			{
				TCHAR arrChar[2];
				int nCharSize = decInputStream.ReadChar(arrChar);
				if (nCharSize <= 0)
					break;
				PutCharToStream(strEntityValue, arrChar, nCharSize);
			}
			decInputStream.SetInputStream(NULL);
			delete pInputStream;
			pInputStream = NULL;
			CHash<CStrStream, CStrStream>& rMapEntities = bDtdEntity ? m_rReader.m_mapDtdEntities : m_rReader.m_mapXmlEntities;
			rMapEntities.SetAt(strEntityName, strEntityValue);
		}
		return XMLR_CONTINUE;
	}
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DtdCondtionHandler(CXmlNode& /*rXmlNode*/, PVOID pParam)
{
	if (m_eParserState == PS_DTD_UNDEFINED)
	{
		m_eParserState = PS_DTD_DOCUMENT;
	}
	else if (m_eParserState != PS_DTD_DOCUMENT && m_eParserState != PS_EMBEDDED_DTD)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	TCHAR arrChar[2];
	int nCharSize;
	CStrStream strCondition(16);
	XML_RESULT eResult;
	for (;;)
	{
		nCharSize = ReadNonSpaceChar(arrChar);
		if (nCharSize <= 0)
			return XMLR_ERROR;
		if (*arrChar != _T('%'))
		{
			PutCharBack(arrChar, nCharSize);
			eResult = ReadName(strCondition);
			if (eResult != XMLR_CONTINUE)
				return XMLR_ERROR;
			break;
		}
		else
		{
			int nValueLength;
			eResult = ReadEntity(nValueLength);
			if (eResult != XMLR_CONTINUE)
				return XMLR_ERROR;
		}
	}
	bool bProcessingEnabled = pParam != NULL;
	if (_tcscmp(strCondition, _T("IGNORE")) == 0)
	{
		bProcessingEnabled = false;
	}
	else if (_tcscmp(strCondition, _T("INCLUDE")) != 0)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	nCharSize = ReadNonSpaceChar(arrChar);
	if (nCharSize <= 0)
		return XMLR_ERROR;
	if (*arrChar != _T('['))
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedChars;
		return XMLR_ERROR;
	}
	++m_nNumEntries;
	eResult = ReadDtdNodes(bProcessingEnabled);
	_ASSERTE(eResult != XMLR_CONTINUE && eResult != XMLR_EOS);
	if (eResult == XMLR_CONTINUE || eResult == XMLR_EOS)
	{
		m_pszErrorMessage = g_pszXmlInternalError;
		return XMLR_ERROR;
	}
	if (eResult != XMLR_EOF)
		return XMLR_ERROR;
	--m_nNumEntries;
	return XMLR_CONTINUE;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DtdCondtionEndHandler(CXmlNode& /*rXmlNode*/, PVOID /*pParam*/)
{
	if (m_eParserState != PS_DTD_DOCUMENT && m_eParserState != PS_EMBEDDED_DTD && m_nNumEntries <= 0)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	return XMLR_EOF;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DtdEndHandler(CXmlNode& /*rXmlNode*/, PVOID /*pParam*/)
{
	if (m_eParserState != PS_EMBEDDED_DTD && m_nNumEntries != 0)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	return XMLR_EOF;
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::DtdDeclarationHandler(CXmlNode& /*rXmlNode*/, PVOID /*pParam*/)
{
	if (m_eParserState == PS_DTD_UNDEFINED)
	{
		m_eParserState = PS_DTD_DOCUMENT;
	}
	else if (m_eParserState != PS_DTD_DOCUMENT && m_eParserState != PS_EMBEDDED_DTD)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedToken;
		return XMLR_ERROR;
	}
	XML_RESULT eResult = SkipUntil(_T(">"));
	return (eResult == XMLR_CONTINUE ? XMLR_EOS : XMLR_ERROR);
}

/**
 * @param rXmlNode - info about current node is stored in this variable.
 * @param rSearchTable - search table.
 * @param pParam - custom defined parameter.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadNextNode(CXmlNode& rXmlNode, CXmlSearchTable& rSearchTable, PVOID pParam)
{
	for (;;)
	{
		XML_RESULT eResult = ProcessNode(rXmlNode, rSearchTable, pParam);
		switch (eResult)
		{
		case XMLR_CONTINUE:
			continue;
		case XMLR_EOS:
			if (rXmlNode.m_eNodeType == CXmlNode::XNT_UNDEFINED)
				continue;
			if (rXmlNode.m_eNodeType == CXmlNode::XNT_TEXT)
			{
				if (m_eParserState != PS_XML_DOCUMENT)
				{
					_ASSERT(FALSE);
					rXmlNode.Reset();
					continue;
				}
				if ((m_rReader.m_dwContentFilter & XCF_WHITESPACES) != 0)
				{
					rXmlNode.m_strNodeValue.Trim();
					if (rXmlNode.m_strNodeValue.IsEmpty())
					{
						rXmlNode.Reset();
						continue;
					}
				}
			}
			break;
		case XMLR_EOF:
			rXmlNode.Reset();
			break;
		}
		return eResult;
	}
}

/**
 * @param bProcessingEnabled - true if DTD processing should be enabled.
 * @return XML result code.
 */
CXmlReader::XML_RESULT CXmlReader::CXmlParser::ReadDtdNodes(bool bProcessingEnabled)
{
	_ASSERTE(
		m_eParserState == PS_DTD_UNDEFINED ||
		m_eParserState == PS_DTD_DOCUMENT ||
		m_eParserState == PS_EMBEDDED_DTD ||
		m_eParserState == PS_XML_PROLOGUE);
	if (m_eParserState == PS_XML_PROLOGUE)
		m_eParserState = PS_EMBEDDED_DTD;
	XML_RESULT eResult;
	CXmlNode xmlNode;
	do
		eResult = ReadNextDtdNode(xmlNode, bProcessingEnabled);
	while (eResult == XMLR_EOS);
	return eResult;
}

/**
 * @param rXmlNode - current node data is stored in this variable.
 * @param dwGotoFlags - go-to flags.
 * @return XML result code.
 */
int CXmlReader::GotoNextElement(CXmlNode& rXmlNode, DWORD dwGotoFlags)
{
	for (;;)
	{
		int iResult = ReadNext(rXmlNode);
		if (iResult == 0)
		{
			if (dwGotoFlags & XGF_ALLOW_ELEMENT_END)
				return XMLR_EOF;
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
			return XMLR_ERROR;
		}
		else if (iResult < 0)
		{
			return XMLR_ERROR;
		}
		if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT ||
			rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_BEGIN)
		{
			return XMLR_EOS;
		}
		else if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
		{
			if (dwGotoFlags & XGF_ALLOW_ELEMENT_END)
				return XMLR_EOS;
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfElement;
			return XMLR_ERROR;
		}
	}
}

/**
 * @param rXmlNode - current node data is stored in this variable.
 * @param dwGotoFlags - go-to flags.
 */
int CXmlReader::GotoNextElementEnd(CXmlNode& rXmlNode, DWORD dwGotoFlags)
{
	int iNestedCount = 0;
	for (;;)
	{
		int iResult = ReadNext(rXmlNode);
		if (iResult == 0)
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
			return XMLR_ERROR;
		}
		else if (iResult < 0)
		{
			return XMLR_ERROR;
		}
		else if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_BEGIN)
		{
			if (iNestedCount > 0 && (dwGotoFlags & XGF_ALLOW_NESTED_ELEMENTS) == 0)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedElement;
				return XMLR_ERROR;
			}
			++iNestedCount;
		}
		else if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
		{
			if (iNestedCount > 0)
				--iNestedCount;
			else
				return XMLR_EOS;
		}
	}
}

/**
 * @param pszName - element name.
 * @param rXmlNode - current node data is stored in this variable.
 * @param dwGotoFlags - go-to flags.
 * @return XML result code.
 */
int CXmlReader::GotoNextElement(PCTSTR pszName, CXmlNode& rXmlNode, DWORD dwGotoFlags)
{
	int iResult = GotoNextElement(rXmlNode, dwGotoFlags);
	if (iResult <= 0)
		return iResult;
	if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
	{
		_ASSERTE(dwGotoFlags & XGF_ALLOW_ELEMENT_END);
		return XMLR_EOS;
	}
	if (_tcscmp(rXmlNode.m_strNodeName, pszName) != 0)
	{
		m_pszErrorMessage = g_pszXmlErrorUnexpectedElement;
		return XMLR_ERROR;
	}
	return XMLR_EOS;
}

/**
 * @param pszName - element name.
 * @param rXmlNode - current node data is stored in this variable.
 * @param dwGotoFlags - go-to flags.
 * @return XML result code.
 */
int CXmlReader::GotoElement(PCTSTR pszName, CXmlNode& rXmlNode, DWORD dwGotoFlags)
{
	CXmlNode XmlNode;
	int iNestedCount = 0;
	for (;;)
	{
		int iResult = ReadNext(XmlNode);
		if (iResult == 0)
		{
			if (dwGotoFlags & XGF_ALLOW_ELEMENT_END)
				return XMLR_EOF;
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
			return XMLR_ERROR;
		}
		else if (iResult < 0)
		{
			return XMLR_ERROR;
		}
		else if (XmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT)
		{
			if (iNestedCount == 0 && _tcscmp(rXmlNode.m_strNodeName, pszName) == 0)
				return XMLR_EOS;
		}
		else if (XmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_BEGIN)
		{
			if (iNestedCount == 0 && _tcscmp(rXmlNode.m_strNodeName, pszName) == 0)
				return XMLR_EOS;
			if (iNestedCount > 0 && (dwGotoFlags & XGF_ALLOW_NESTED_ELEMENTS) == 0)
			{
				m_pszErrorMessage = g_pszXmlErrorUnexpectedElement;
				return XMLR_ERROR;
			}
			++iNestedCount;
		}
		else if (XmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
		{
			if (iNestedCount > 0)
			{
				--iNestedCount;
			}
			else
			{
				if (dwGotoFlags & XGF_ALLOW_ELEMENT_END)
					return XMLR_EOS;
				m_pszErrorMessage = g_pszXmlErrorInvalidClosingElement;
				return XMLR_ERROR;
			}
		}
	}
}

/**
 * @param rXmlNode - current node data is stored in this variable.
 * @param dwGotoFlags - go-to flags.
 * @return XML result code.
 */
int CXmlReader::GotoText(CXmlNode& rXmlNode, DWORD dwGotoFlags)
{
	for (;;)
	{
		int iResult = ReadNext(rXmlNode);
		if (iResult == 0)
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfStream;
			return XMLR_ERROR;
		}
		else if (iResult < 0)
		{
			return XMLR_ERROR;
		}
		else if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT ||
			rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_BEGIN)
		{
			m_pszErrorMessage = g_pszXmlErrorUnexpectedElement;
			return XMLR_ERROR;
		}
		else if (rXmlNode.m_eNodeType == CXmlNode::XNT_ELEMENT_END)
		{
			if (dwGotoFlags & XGF_ALLOW_ELEMENT_END)
				return XMLR_EOS;
			m_pszErrorMessage = g_pszXmlErrorUnexpectedEndOfElement;
			return XMLR_ERROR;
		}
		else if (rXmlNode.m_eNodeType == CXmlNode::XNT_TEXT)
		{
			return XMLR_EOS;
		}
	}
}
