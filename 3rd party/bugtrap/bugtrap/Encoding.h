/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Text encoding/decoding.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "Stream.h"
#include "StrStream.h"

/// Lower value of Unicode high-surrogate character.
#define HIGH_SURROGATE_START   0xD800
/// Higher value of Unicode high-surrogate character.
#define HIGH_SURROGATE_END     0xDBFF
/// Lower value of Unicode low-surrogate character.
#define LOW_SURROGATE_START    0xDC00
/// Higher value of Unicode low-surrogate character.
#define LOW_SURROGATE_END      0xDFFF

/// Text encoding.
enum TEXT_ENCODING
{
	/// Ansi code page.
	TXTENC_ANSI,
	/// UTF-8 encoding.
	TXTENC_UTF8,
	/// UTF-16/UCS-2, little-endian encoding.
	TXTENC_UTF16LE,
	/// UTF-16/UCS-2, big-endian encoding.
	TXTENC_UTF16BE
};

/// UTF-8 encoder.
class CUTF8EncStream
{
public:
	/// Initialize the object.
	CUTF8EncStream(void);
	/// Initialize the object.
	explicit CUTF8EncStream(CStream* pStream);
	/// Initialize the object.
	explicit CUTF8EncStream(COutputStream* pOutputStream);
	/// Get data stream.
	CStream* GetStream(void) const;
	/// Get data stream.
	COutputStream* GetOutputStream(void) const;
	/// Set data stream.
	void SetStream(CStream* pStream);
	/// Set data stream.
	void SetOutputStream(COutputStream* pOutputStream);
	/// Write data to the buffer.
	bool WriteByte(BYTE bValue);
	/// Write data to the buffer.
	bool WriteByte(BYTE bValue, int nCount);
	/// Write data to the buffer.
	bool WriteBytes(const BYTE* pBytes, int nCount);
	/// Write data to the buffer.
	bool WriteAscii(PCSTR pszString);
	/// Write data to the buffer.
	bool WriteUTF8Bin(const TCHAR* pchValue, int& nCharSize);
	/// Write data to the buffer.
	bool WriteUTF8Bin(PCTSTR pszString);
	/// Write data to the buffer.
	bool WriteUTF8Hex(const TCHAR* pchValue, int& nCharSize);
	/// Write data to the buffer.
	bool WriteUTF8Hex(PCTSTR pszString);
	/// Write other pre-encoded data to the buffer.
	bool Write(const CUTF8EncStream& rEncStream);
	/// Clear data in the buffer.
	void Reset(void);

private:
	/// Object can't be copied.
	CUTF8EncStream(const CUTF8EncStream& rStream);
	/// Object can't be copied.
	CUTF8EncStream& operator=(const CUTF8EncStream& rStream);

	/// Write Unicode character value in UTF-8 encoding.
	bool WriteUTF8Bin(DWORD dwUnicodeChar);
	/// Write Unicode character value in UTF-8 encoding.
	bool WriteUTF8Hex(DWORD dwUnicodeChar);

	/// Data stream.
	CStream* m_pStream;
	/// Data stream.
	COutputStream* m_pOutputStream;
};

inline CUTF8EncStream::CUTF8EncStream(void) : m_pStream(NULL), m_pOutputStream(NULL)
{
}

/**
 * @param pStream - data stream.
 */
inline CUTF8EncStream::CUTF8EncStream(CStream* pStream) : m_pStream(pStream), m_pOutputStream(pStream)
{
}

/**
 * @param pOutputStream - data stream.
 */
inline CUTF8EncStream::CUTF8EncStream(COutputStream* pOutputStream) : m_pStream(NULL), m_pOutputStream(pOutputStream)
{
}

/**
 * @return data stream.
 */
inline CStream* CUTF8EncStream::GetStream(void) const
{
	return m_pStream;
}

/**
 * @return data stream.
 */
inline COutputStream* CUTF8EncStream::GetOutputStream(void) const
{
	return m_pOutputStream;
}

/**
 * @param pStream - data stream.
 */
inline void CUTF8EncStream::SetStream(CStream* pStream)
{
	m_pStream = pStream;
	m_pOutputStream = pStream;
}

/**
 * @param pOutputStream - data stream.
 */
inline void CUTF8EncStream::SetOutputStream(COutputStream* pOutputStream)
{
	m_pStream = NULL;
	m_pOutputStream = pOutputStream;
}

/**
 * @param pszString - string to be written.
 * @return true if data has been written.
 */
inline bool CUTF8EncStream::WriteAscii(PCSTR pszString)
{
	_ASSERTE(m_pOutputStream != NULL);
	int nLength = strlen(pszString);
	return (m_pOutputStream->WriteBytes((const BYTE*)pszString, nLength) == nLength);
}

/**
 * @param pBytes - bytes array to be written.
 * @param nCount - number of bytes to be added.
 * @return true if data has been written.
 */
inline bool CUTF8EncStream::WriteBytes(const BYTE* pBytes, int nCount)
{
	_ASSERTE(m_pOutputStream != NULL);
	return (m_pOutputStream->WriteBytes(pBytes, nCount) == nCount);
}

/**
 * @param bValue - byte value to be written.
 * @param nCount - number of bytes to be added.
 * @return true if data has been written.
 */
inline bool CUTF8EncStream::WriteByte(BYTE bValue, int nCount)
{
	_ASSERTE(m_pOutputStream != NULL);
	return (m_pOutputStream->WriteByte(bValue, nCount) == nCount);
}

/**
 * @param bValue - byte value to be written.
 * @return true if data has been written.
 */
inline bool CUTF8EncStream::WriteByte(BYTE bValue)
{
	_ASSERTE(m_pOutputStream != NULL);
	return m_pOutputStream->WriteByte(bValue);
}

inline void CUTF8EncStream::Reset(void)
{
	if (m_pOutputStream != NULL)
	{
		_ASSERTE((m_pOutputStream->GetFeatures() & COutputStream::SF_SETLENGTH) != 0);
		m_pOutputStream->SetLength(0);
	}
}

/**
 * @param rEncStream - another encoder object.
 * @return true if data has been written.
 */
inline bool CUTF8EncStream::Write(const CUTF8EncStream& rEncStream)
{
	_ASSERTE(m_pOutputStream != NULL);
	_ASSERTE(rEncStream.m_pStream != NULL);
	return (m_pOutputStream->WriteStream(rEncStream.m_pStream) >= 0);
}

// Other encoding/decoding functions.
int Read7BitEncodedInt(PBYTE pBuffer, int& nPosition, int nBufferSize);
bool Write7BitEncodedInt(int nValue, PBYTE pBuffer, int& nPosition, int nBufferSize);
bool WriteBinaryString(CUTF8EncStream& rEncStream, PCTSTR pszString, PBYTE pBuffer, int& nPosition, int nBufferSize);
bool WriteBinaryString(PCTSTR pszString, PBYTE pBuffer, int& nPosition, int nBufferSize);
int UTF8DecodeChar(const BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize);
int UTF16BeDecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize);
int UTF16LeDecodeChar(const BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize);
int AnsiDecodeChar(const BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize);
int UTF8DecodeString(const BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize);
int UTF16BeDecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize);
int UTF16LeDecodeString(const BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize);
int AnsiDecodeString(const BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize);
int GetUTF8CharSize(const BYTE* pBytes);
int GetUnicodeCharSize(const BYTE* pBytes);
bool IsUnicodeLeadChar(const BYTE* pBytes);
int UTF16BeToLeChar(BYTE* pBytes, int nNumBytes);
int UTF16BeToLeString(BYTE* pBytes, int nNumBytes);
int GetCharSizeInUTF8(const TCHAR* pchValue, int& nCharSize);
int GetStringSizeInUTF8(PCTSTR pszString);

/// Base decoder interface.
class CBaseDecoder
{
public:
	/// Object destructor.
	virtual ~CBaseDecoder(void) { }
	/// Return current encoding.
	virtual TEXT_ENCODING GetEncoding(void) = 0;
	/// Decode single character.
	virtual int DecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize) = 0;
	/// Decode string of characters.
	virtual int DecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize) = 0;
	/// Get appropriate decoder base.
	static CBaseDecoder* GetDecoder(TEXT_ENCODING eEncoding);
	/// Get appropriate decoder base.
	static CBaseDecoder* GetDecoder(PCTSTR pszEncoding);
};

/// UTF-8 decoder.
class CUTF8Decoder : public CBaseDecoder
{
public:
	/// Return current encoding.
	virtual TEXT_ENCODING GetEncoding(void)
	{ return TXTENC_UTF8; }
	/// Decode single character.
	virtual int DecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
	{ return UTF8DecodeChar(pBytes, nNumBytes, arrChar, nCharSize); }
	/// Decode string of characters.
	virtual int DecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
	{ return UTF8DecodeString(pBytes, nNumBytes, pszString, nBufferSize); }
	/// Get single object instance.
	static CUTF8Decoder& GetInstance(void)
	{ return m_instance; }

private:
	/// Hides object constructor.
	CUTF8Decoder(void) { }
	/// Object can't be copied.
	CUTF8Decoder(const CUTF8Decoder& rDecoder);
	/// Object can't be copied.
	CUTF8Decoder& operator=(const CUTF8Decoder& rDecoder);

	/// Single object instance.
	static CUTF8Decoder m_instance;
};

/// UTF-16 decoder.
class CUTF16LeDecoder : public CBaseDecoder
{
public:
	/// Return current encoding.
	virtual TEXT_ENCODING GetEncoding(void)
	{ return TXTENC_UTF16LE; }
	/// Decode single character.
	virtual int DecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
	{ return UTF16LeDecodeChar(pBytes, nNumBytes, arrChar, nCharSize); }
	/// Decode string of characters.
	virtual int DecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
	{ return UTF16LeDecodeString(pBytes, nNumBytes, pszString, nBufferSize); }
	/// Get single object instance.
	static CUTF16LeDecoder& GetInstance(void)
	{ return m_instance; }

private:
	/// Hides object constructor.
	CUTF16LeDecoder(void) { }
	/// Object can't be copied.
	CUTF16LeDecoder(const CUTF16LeDecoder& rDecoder);
	/// Object can't be copied.
	CUTF16LeDecoder& operator=(const CUTF16LeDecoder& rDecoder);

	/// Single object instance.
	static CUTF16LeDecoder m_instance;
};

/// UTF-16 decoder.
class CUTF16BeDecoder : public CBaseDecoder
{
public:
	/// Return current encoding.
	virtual TEXT_ENCODING GetEncoding(void)
	{ return TXTENC_UTF16BE; }
	/// Decode single character.
	virtual int DecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
	{ return UTF16BeDecodeChar(pBytes, nNumBytes, arrChar, nCharSize); }
	/// Decode string of characters.
	virtual int DecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
	{ return UTF16BeDecodeString(pBytes, nNumBytes, pszString, nBufferSize); }
	/// Get single object instance.
	static CUTF16BeDecoder& GetInstance(void)
	{ return m_instance; }

private:
	/// Hides object constructor.
	CUTF16BeDecoder(void) { }
	/// Object can't be copied.
	CUTF16BeDecoder(const CUTF16BeDecoder& rDecoder);
	/// Object can't be copied.
	CUTF16BeDecoder& operator=(const CUTF16BeDecoder& rDecoder);

	/// Single object instance.
	static CUTF16BeDecoder m_instance;
};

/// Ansi decoder.
class CAnsiDecoder : public CBaseDecoder
{
public:
	/// Return current encoding.
	virtual TEXT_ENCODING GetEncoding(void)
	{ return TXTENC_ANSI; }
	/// Decode single character.
	virtual int DecodeChar(BYTE* pBytes, int nNumBytes, TCHAR arrChar[2], int& nCharSize)
	{ return AnsiDecodeChar(pBytes, nNumBytes, arrChar, nCharSize); }
	/// Decode string of characters.
	virtual int DecodeString(BYTE* pBytes, int nNumBytes, PTSTR pszString, int nBufferSize)
	{ return AnsiDecodeString(pBytes, nNumBytes, pszString, nBufferSize); }
	/// Get single object instance.
	static CAnsiDecoder& GetInstance(void)
	{ return m_instance; }

private:
	/// Hides object constructor.
	CAnsiDecoder(void) { }
	/// Object can't be copied.
	CAnsiDecoder(const CAnsiDecoder& rDecoder);
	/// Object can't be copied.
	CAnsiDecoder& operator=(const CAnsiDecoder& rDecoder);

	/// Single object instance.
	static CAnsiDecoder m_instance;
};

/// Text input stream.
class CCharInputStream
{
public:
	/// Object destructor.
	virtual ~CCharInputStream(void) { }
	/// Read character from the stream.
	virtual int ReadChar(TCHAR arrChar[2]) = 0;
	/// Get byte order mark. This function is optional.
	virtual bool ReadPreamble(TEXT_ENCODING& /*eEncoding*/)
	{ return false; }
	/// Get text decoder. This function is optional.
	virtual CBaseDecoder* GetDecoder(void) const
	{ return NULL; }
	/// Set text decoder. This function is optional.
	virtual bool SetDecoder(CBaseDecoder* /*pDecoder*/)
	{ return false; }
	/// Get stream name. This function is optional.
	virtual bool GetName(PTSTR pszName, int nNameSize) const
	{ if (nNameSize > 0) *pszName = _T('0'); return false; }
	/// Read and apply byte order mark.
	bool CheckEncoding(void);
	/// Read and apply byte order mark.
	bool CheckEncoding(TEXT_ENCODING eDefaultEncoding);
};

/// Decoding input stream.
class CDecInputStream : public CCharInputStream
{
public:
	/// Initialize the object.
	explicit CDecInputStream(CInputStream* pInputStream = NULL);
	/// Read character from the stream.
	virtual int ReadChar(TCHAR arrChar[2]);
	/// Get byte order mark. This function is optional.
	virtual bool ReadPreamble(TEXT_ENCODING& eEncoding);
	/// Get text decoder.
	virtual CBaseDecoder* GetDecoder(void) const;
	/// Set text decoder.
	virtual bool SetDecoder(CBaseDecoder* pDecoder);
	/// Get stream name.
	virtual bool GetName(PTSTR pszName, int nNameSize) const;
	/// Attach input stream to the object.
	void SetInputStream(CInputStream* pInputStream);
	/// Get pointer to the input stream.
	CInputStream* GetInputStream(void) const;

private:
	/// Object can't be copied.
	CDecInputStream(const CDecInputStream& rStream);
	/// Object can't be copied.
	CDecInputStream& operator=(const CDecInputStream& rStream);
	/// Fill buffer with bytes.
	int FillBuffer(int nNumBytes);

	/// Input stream attached to the object.
	CInputStream* m_pInputStream;
	/// Text decoder.
	CBaseDecoder* m_pDecoder;
	/// Binary buffer for encoded data.
	BYTE m_arrInputBuffer[1024];
	/// Position in the buffer.
	int m_nInputBufferPos;
	/// Number of bytes in the buffer.
	int m_nInputBufferLength;
	/// True after reaching end of file.
	bool m_bEndOfFile;
};

/**
 * @param pInputStream - input stream.
 */
inline CDecInputStream::CDecInputStream(CInputStream* pInputStream)
{
	SetDecoder(&CAnsiDecoder::GetInstance());
	SetInputStream(pInputStream);
}

/**
 * @return pointer to input stream.
 */
inline CInputStream* CDecInputStream::GetInputStream(void) const
{
	return m_pInputStream;
}

/**
 * @return pointer to current decoder or @a NULL.
 */
inline CBaseDecoder* CDecInputStream::GetDecoder(void) const
{
	return m_pDecoder;
}

/**
 * @param pDecoder - pointer to the decoder or @a NULL.
 * @return true if decoding was successfully applied.
 */
inline bool CDecInputStream::SetDecoder(CBaseDecoder* pDecoder)
{
	_ASSERTE(pDecoder != NULL);
	m_pDecoder = pDecoder;
	return true;
}

/**
 * @param pszName - stream name buffer.
 * @param nNameSize - size of stream name buffer.
 * @return true if name was retrieved.
 */
inline bool CDecInputStream::GetName(PTSTR pszName, int nNameSize) const
{
	_ASSERTE(m_pInputStream != NULL);
	return m_pInputStream->GetName(pszName, nNameSize);
}

/// String input stream.
class CStrInputStream : public CCharInputStream
{
public:
	/// Initialize the object.
	explicit CStrInputStream(const CStrStream* pStrStream = NULL);
	/// Read character from the stream.
	virtual int ReadChar(TCHAR arrChar[2]);
	/// Attach input stream to the object.
	void SetStrStream(const CStrStream* pStrStream);
	/// Get pointer to the input stream.
	const CStrStream* GetStrStream(void) const;

private:
	/// Object can't be copied.
	CStrInputStream(const CStrInputStream& rStream);
	/// Object can't be copied.
	CStrInputStream& operator=(const CStrInputStream& rStream);

	/// Stream attached to the object.
	const CStrStream* m_pStrStream;
	/// Current position in the stream.
	int m_nPosition;
};

/**
 * @param pStrStream - input stream.
 */
inline CStrInputStream::CStrInputStream(const CStrStream* pStrStream)
{
	SetStrStream(pStrStream);
}

/**
 * @return pointer to input stream.
 */
inline const CStrStream* CStrInputStream::GetStrStream(void) const
{
	return m_pStrStream;
}

/**
 * @param pStrStream - input stream.
 */
inline void CStrInputStream::SetStrStream(const CStrStream* pStrStream)
{
	m_pStrStream = pStrStream;
	m_nPosition = 0;
}
