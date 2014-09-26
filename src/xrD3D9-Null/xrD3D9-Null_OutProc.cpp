#include "stdafx.h"

HRESULT		HRESULT_Proc(HRESULT ret)
{
	return ret;
};

ULONG		ULONG_Proc(ULONG ret)
{
	return ret;
};

UINT		UINT_Proc(UINT ret)
{
	return ret;
};

BOOL		BOOL_Proc(BOOL ret)
{
	return ret;
};

float		FLOAT_Proc(float ret)
{
	return ret;
};
void		VOID_proc()
{
	int x=0;
	x=x;
};

DWORD		DWORD_Proc(DWORD ret)
{
	return ret;
}

//-----------------------------------------------------------------------
#include "stdio.h"
void LogOut( const char *format, ... ) 
{
	va_list argptr;
	char text[4096];

	va_start (argptr,format);
	vsprintf (text, format, argptr);
	va_end (argptr);

	//rr  printf(text);
//	OutputDebugString( text );
}

void LogOut_File(const char *pszFormat, ...)
{
	char s[128];
	va_list va;
	va_start(va, pszFormat);
	vsprintf(s, pszFormat, va);
	va_end(va);
	fprintf(stderr,s);
	

	FILE *fp;
	fp = fopen("d3d9-null.log","a+t");
	if (fp)
	{		
		fprintf(fp,"%s",s);
		fclose(fp);
	}
}
