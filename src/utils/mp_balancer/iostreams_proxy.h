#ifndef IOSTREAMS_PROXY_INCLUDED
#define IOSTREAMS_PROXY_INCLUDED

#include <stdio.h>
#include <conio.h>

#ifndef _STLP_NO_IOSTREAMS
#	include <iostream>
#else //#ifdef _STLP_NO_IOSTREAMS

namespace std
{

class console_output
{
public:
	console_output()	{};
	~console_output()	{};

	console_output & operator << (char const * str)
	{
		printf(str);
		return *this;
	}
};//class console_output

extern console_output	cout;
extern console_output	cerr;
extern console_output	clog;
extern char const *		endl;

}; //namespace std

#endif //#ifndef _STLP_NO_IOSTREAMS

#endif //#ifndef IOSTREAMS_PROXY_INCLUDED