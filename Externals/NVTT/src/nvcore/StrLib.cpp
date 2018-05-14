// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/StrLib.h>

#include <math.h>	// log
#include <stdio.h>	// vsnprintf

#if NV_CC_MSVC
#include <stdarg.h> // vsnprintf
#endif

#if NV_OS_WIN32
#define NV_PATH_SEPARATOR '\\'
#else
#define NV_PATH_SEPARATOR '/'
#endif

using namespace nv;

namespace 
{
	static char * strAlloc(uint size)
	{
		return static_cast<char *>(mem::malloc(size));
	}
	
	static char * strReAlloc(char * str, uint size)
	{
		return static_cast<char *>(mem::realloc(str, size));
	}
	
	static void strFree(const char * str)
	{
		return mem::free(const_cast<char *>(str));
	}
	
	/*static char * strDup( const char * str ) 
	{
		nvDebugCheck( str != NULL );
		uint len = uint(strlen( str ) + 1);
		char * dup = strAlloc( len );
		memcpy( dup, str, len );
		return dup;
	}*/
	
	// helper function for integer to string conversion.
	static char * i2a( uint i, char *a, uint r )
	{
		if( i / r > 0 ) {
			a = i2a( i / r, a, r );
		}
		*a = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % r];
		return a + 1;
	}
	
	// Locale independent functions.
	static inline char toUpper( char c ) {
		return (c<'a' || c>'z') ? (c) : (c+'A'-'a');
	}
	static inline char toLower( char c ) {
		return (c<'A' || c>'Z') ? (c) : (c+'a'-'A');
	}
	static inline bool isAlpha( char c ) {
		return (c>='a' && c<='z') || (c>='A' && c<='Z');
	}
	static inline bool isDigit( char c ) {
		return c>='0' && c<='9';
	}
	static inline bool isAlnum( char c ) {
		return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
	}
	
}

int nv::strCmp(const char * s1, const char * s2)
{
	nvDebugCheck(s1 != NULL);
	nvDebugCheck(s2 != NULL);
	return strcmp(s1, s2);
}

int nv::strCaseCmp(const char * s1, const char * s2)
{
	nvDebugCheck(s1 != NULL);
	nvDebugCheck(s1 != NULL);
#if NV_CC_MSVC
	return _stricmp(s1, s2);
#else
	return strcasecmp(s1, s2);
#endif
}

void nv::strCpy(char * dst, int size, const char * src)
{
	nvDebugCheck(dst != NULL);
	nvDebugCheck(src != NULL);
#if NV_CC_MSVC && _MSC_VER >= 1400
	strcpy_s(dst, size, src);
#else
	NV_UNUSED(size);
	strcpy(dst, src);
#endif
}

void nv::strCpy(char * dst, int size, const char * src, int len)
{
	nvDebugCheck(dst != NULL);
	nvDebugCheck(src != NULL);
#if NV_CC_MSVC && _MSC_VER >= 1400
	strncpy_s(dst, size, src, len);
#else
	NV_UNUSED(size);
	strncpy(dst, src, len);
#endif
}

void nv::strCat(char * dst, int size, const char * src)
{
	nvDebugCheck(dst != NULL);
	nvDebugCheck(src != NULL);
#if NV_CC_MSVC && _MSC_VER >= 1400
	strcat_s(dst, size, src);
#else
	NV_UNUSED(size);
	strcat(dst, src);
#endif
}


/** Pattern matching routine. I don't remember where did I get this. */
bool nv::strMatch(const char * str, const char * pat)
{
	nvDebugCheck(str != NULL);
	nvDebugCheck(pat != NULL);

    char c2;

    while (true) {
        if (*pat==0) {
            if (*str==0) return true;
            else         return false;
        }
        if ((*str==0) && (*pat!='*')) return false;
        if (*pat=='*') {
            pat++;
            if (*pat==0) return true;
            while (true) {
                if (strMatch(str, pat)) return true;
                if (*str==0) return false;
                str++;
            }
        }
        if (*pat=='?') goto match;
        if (*pat=='[') {
            pat++;
            while (true) {
                if ((*pat==']') || (*pat==0)) return false;
                if (*pat==*str) break;
                if (pat[1] == '-') {
                    c2 = pat[2];
                    if (c2==0) return false;
                    if ((*pat<=*str) && (c2>=*str)) break;
                    if ((*pat>=*str) && (c2<=*str)) break;
                    pat+=2;
                }
                pat++;
            }
            while (*pat!=']') {
                if (*pat==0) {
                    pat--;
                    break;
                }
                pat++;
            }
            goto match;
        }

        if (*pat == NV_PATH_SEPARATOR) {
            pat++;
            if (*pat==0) return false;
        }
        if (*pat!=*str) return false;

match:
        pat++;
        str++;
    }
}



/** Empty string. */
StringBuilder::StringBuilder() : m_size(0), m_str(NULL)
{
}

/** Preallocate space. */
StringBuilder::StringBuilder( int size_hint ) : m_size(size_hint)
{
	nvDebugCheck(m_size > 0);
	m_str = strAlloc(m_size);
	*m_str = '\0';
}

/** Copy ctor. */
StringBuilder::StringBuilder( const StringBuilder & s ) : m_size(0), m_str(NULL)
{
	copy(s);
}

/** Copy string. */
StringBuilder::StringBuilder( const char * s ) : m_size(0), m_str(NULL)
{
	copy(s);
}

/** Delete the string. */
StringBuilder::~StringBuilder()
{
	m_size = 0;
	strFree(m_str);
	m_str = NULL;
}


/** Format a string safely. */
StringBuilder & StringBuilder::format( const char * fmt, ... )
{
	nvDebugCheck(fmt != NULL);
	va_list arg;
	va_start( arg, fmt );

	format( fmt, arg );

	va_end( arg );

	return *this;
}


/** Format a string safely. */
StringBuilder & StringBuilder::format( const char * fmt, va_list arg )
{
	nvDebugCheck(fmt != NULL);

	if( m_size == 0 ) {
		m_size = 64;
		m_str = strAlloc( m_size );
	}

	va_list tmp;
	va_copy(tmp, arg);
#if NV_CC_MSVC && _MSC_VER >= 1400
	int n = vsnprintf_s(m_str, m_size, _TRUNCATE, fmt, tmp);
#else
	int n = vsnprintf(m_str, m_size, fmt, tmp);
#endif
	va_end(tmp);

	while( n < 0 || n >= int(m_size) ) {
		if( n > -1 ) {
			m_size = n + 1;
		}
		else {
			m_size *= 2;
		}

		m_str = strReAlloc(m_str, m_size);

		va_copy(tmp, arg);
#if NV_CC_MSVC && _MSC_VER >= 1400
		n = vsnprintf_s(m_str, m_size, _TRUNCATE, fmt, tmp);
#else
		n = vsnprintf(m_str, m_size, fmt, tmp);
#endif
		va_end(tmp);
	}
	
	nvDebugCheck(n < int(m_size));
	
	// Make sure it's null terminated.
	nvDebugCheck(m_str[n] == '\0');
	//str[n] = '\0';

	return *this;
}


/** Append a string. */
StringBuilder & StringBuilder::append( const char * s )
{
	nvDebugCheck(s != NULL);

	const uint slen = uint(strlen( s ));

	if( m_str == NULL ) {
		m_size = slen + 1;
		m_str = strAlloc(m_size);
		strCpy( m_str, m_size, s );
	}
	else {
	
		const uint len = uint(strlen( m_str ));

		if( m_size < len + slen + 1 ) {
			m_size = len + slen + 1;
			m_str = strReAlloc(m_str, m_size);
		}
		
		strCat( m_str, m_size, s );
	}

	return *this;
}


/** Append a formatted string. */
StringBuilder & StringBuilder::appendFormat( const char * format, ... )
{
	nvDebugCheck( format != NULL );

	va_list arg;
	va_start( arg, format );

	appendFormat( format, arg );

	va_end( arg );

	return *this;
}


/** Append a formatted string. */
StringBuilder & StringBuilder::appendFormat( const char * format, va_list arg )
{
	nvDebugCheck( format != NULL );
	
	va_list tmp;
	va_copy(tmp, arg);

	StringBuilder tmp_str;
	tmp_str.format( format, tmp );
	append( tmp_str );
	
	va_end(tmp);

	return *this;
}


/** Convert number to string in the given base. */
StringBuilder & StringBuilder::number( int i, int base )
{
	nvCheck( base >= 2 );
	nvCheck( base <= 36 );

	// @@ This needs to be done correctly.
	// length = floor(log(i, base));
	uint len = uint(log(float(i)) / log(float(base)) + 2);	// one more if negative
	reserve(len);

	if( i < 0 ) {
		*m_str = '-';
		*i2a(uint(-i), m_str+1, base) = 0;
	}
	else {
		*i2a(i, m_str, base) = 0;
	}

	return *this;
}


/** Convert number to string in the given base. */
StringBuilder & StringBuilder::number( uint i, int base )
{
	nvCheck( base >= 2 );
	nvCheck( base <= 36 );

	// @@ This needs to be done correctly.
	// length = floor(log(i, base));
	uint len = uint(log(float(i)) / log(float(base)) - 0.5f + 1);
	reserve(len);

	*i2a(i, m_str, base) = 0;

	return *this;
}


/** Resize the string preserving the contents. */
StringBuilder & StringBuilder::reserve( uint size_hint )
{
	nvCheck(size_hint != 0);
	if( size_hint > m_size ) {
		m_str = strReAlloc(m_str, size_hint);
		m_size = size_hint;
	}
	return *this;
}


/** Copy a string safely. */
StringBuilder & StringBuilder::copy( const char * s )
{
	nvCheck( s != NULL );
	uint str_size = uint(strlen( s )) + 1;
	reserve(str_size);
	strCpy( m_str, str_size, s );
	return *this;
}


/** Copy an StringBuilder. */
StringBuilder & StringBuilder::copy( const StringBuilder & s )
{
	if( s.m_str == NULL ) {
		nvCheck( s.m_size == 0 );
		m_size = 0;
		strFree( m_str );
		m_str = NULL;
	}
	else {
		reserve( s.m_size );
		strCpy( m_str, s.m_size, s.m_str );
	}
	return *this;
}

/** Reset the string. */
void StringBuilder::reset()
{
	m_size = 0;
	strFree( m_str );
	m_str = NULL;
}


/// Get the file name from a path.
const char * Path::fileName() const
{
	return fileName(m_str);
}


/// Get the extension from a file path.
const char * Path::extension() const
{
	return extension(m_str);
}


/// Toggles path separators (ie. \\ into /).
void Path::translatePath()
{
	nvCheck( m_str != NULL );

	for(int i = 0; ; i++) {
		if( m_str[i] == '\0' ) break;
#if NV_PATH_SEPARATOR == '/'
		if( m_str[i] == '\\' ) m_str[i] = NV_PATH_SEPARATOR;
#else
		if( m_str[i] == '/' ) m_str[i] = NV_PATH_SEPARATOR;
#endif
	}
}


/**
 * Strip the file name from a path.
 * @warning path cannot end with '/' o '\\', can't it?
 */
void Path::stripFileName()
{
	nvCheck( m_str != NULL );

	int length = (int)strlen(m_str) - 1;
	while (length > 0 && m_str[length] != '/' && m_str[length] != '\\'){
		length--;
	}
	if( length ) {
		m_str[length+1] = 0;
	}
	else {
		m_str[0] = 0;
	}
}


/// Strip the extension from a path name.
void Path::stripExtension()
{
	nvCheck( m_str != NULL );
	
	int length = (int)strlen(m_str) - 1;
	while( length > 0 && m_str[length] != '.' ) {
		length--;
		if( m_str[length] == NV_PATH_SEPARATOR ) {
			return;		// no extension
		}
	}
	if( length ) {
		m_str[length] = 0;
	}
}


/// Get the path separator.
// static
char Path::separator()
{
	return NV_PATH_SEPARATOR;
}

// static 
const char * Path::fileName(const char * str)
{
	nvCheck( str != NULL );

	int length = (int)strlen(str) - 1;
	while( length >= 0 && str[length] != separator() ) {
		length--;
	}

	return &str[length+1];
}

// static 
const char * Path::extension(const char * str)
{
	nvCheck( str != NULL );

	int length, l;
	l = length = (int)strlen( str );
	while( length > 0 && str[length] != '.' ) {
		length--;
		if( str[length] == separator() ) {
			return &str[l];		// no extension
		}
	}
	if( length == 0 ) {
		return &str[l];
	}
	return &str[length];
}



/// Clone this string
String String::clone() const
{
	String str(data);
	return str;
}

void String::setString(const char * str)
{
	if (str == NULL) {
		data = NULL;
	}
	else {
		allocString( str );
		addRef();
	}
}

void String::setString(const char * str, int length)
{
	nvDebugCheck(str != NULL);

	allocString(str, length);
	addRef();
}

void String::setString(const StringBuilder & str)
{
	if (str.str() == NULL) {
		data =	NULL;
	}
	else {
		allocString(str);
		addRef();
	}
}	
