#include "stdafx.h"
#include <stdio.h>


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
	fputs(s,stderr);

	FILE *fp;
	fp = fopen("d3d9-null.log","a+t");
	if (fp)
	{		
		fprintf(fp,"%s",s);
		fclose(fp);
	}
}
