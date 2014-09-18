#pragma once
#ifdef	DEBUG
XRCORE_API std::string get_string( bool v );
XRCORE_API std::string get_string( const Fvector& v );
XRCORE_API std::string get_string( const Fmatrix& dop );
XRCORE_API std::string get_string(const Fbox &box);

XRCORE_API std::string dump_string( LPCSTR name, const Fvector &v );
XRCORE_API std::string dump_string( LPCSTR name, const Fmatrix &form );
XRCORE_API void dump( LPCSTR name, const Fmatrix &form );
XRCORE_API void dump( LPCSTR name, const Fvector &v );

#endif