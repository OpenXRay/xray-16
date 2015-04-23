extern	void		LogOut( const char *format, ... );
extern	void		LogOut_File( const char *format, ... );

#ifdef _DEBUG
#define APIDEBUG(str)LogOut("---------------------"#str"-------------------------\n")//; LogOut_File("---------------------"#str"-------------------------\n")

#else
#define APIDEBUG(str) // LogOut_File("---------------------"#str"-------------------------\n")

#endif