// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_TOKENIZER_H
#define NV_CORE_TOKENIZER_H

#include <nvcore/nvcore.h>
#include <nvcore/Stream.h>
#include <nvcore/TextReader.h>
#include <nvcore/StrLib.h>

namespace nv
{
	/// A token produced by the Tokenizer.
	class NVCORE_CLASS Token
	{
	public:
		Token();
		Token(const Token & token);
		Token(const char * str, int len);		
		
		bool operator==(const char * str) const;
		bool operator!=(const char * str) const;

		bool isNull();
		
		float toFloat() const;
		int toInt() const;
		uint toUnsignedInt() const;
		String toString() const;
		
		bool parse(const char * format, int count, ...) const __attribute__((format (scanf, 2, 4)));
		
	private:
		const char * m_str;
		int m_len;
	};
	
	/// Exception thrown by the tokenizer.
	class TokenizerException
	{
	public:
		TokenizerException(int line, int column) : m_line(line), m_column(column) {}
		
		int line() const { return m_line; }
		int column() const { return m_column; }
		
	private:
		int m_line;
		int m_column;
	};
	
	// @@ Use enums instead of bools for clarity!
	//enum SkipEmptyLines { skipEmptyLines, noSkipEmptyLines };
	//enum SkipEndOfLine { skipEndOfLine, noSkipEndOfLine };

	/// A simple stream tokenizer.
	class NVCORE_CLASS Tokenizer
	{
	public:
		Tokenizer(Stream * stream);
		
		bool nextLine(bool skipEmptyLines = true);
		bool nextToken(bool skipEndOfLine = false);
		
		const Token & token() const { return m_token; }
		
		int lineNumber() const { return m_lineNumber; }
		int columnNumber() const { return m_columnNumber; }
		
		void setDelimiters(const char * str) { m_delimiters = str; }
		const char * delimiters() const { return m_delimiters; }
		
		void setSpaces(const char * str) { m_spaces = str; }
		const char * spaces() const { return m_spaces; }
		
	private:
		char readChar();
		bool readLine();
		bool readToken(); 
		void skipSpaces();
		bool isSpace(char c);
		bool isDelimiter(char c);
		
	private:
		TextReader m_reader;
		const char * m_line;
		Token m_token;
		
		int m_lineNumber;
		int m_columnNumber;
		
		const char * m_delimiters;
		const char * m_spaces;
	};
	
} // nv namespace


#endif // NV_CORE_TOKENIZER_H
