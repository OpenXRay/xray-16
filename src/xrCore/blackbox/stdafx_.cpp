// stdafx.cpp : source file that includes just the standard includes
//	BlackBox.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx_.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

namespace BlackBox {


bool isspace( int ch )
{
	return ((ch == 0x09) || (ch == 0x0A) || (ch == 0x0B) || 
		(ch == 0x0B) || (ch == 0x0C) || (ch == 0x0D) || (ch == 0x20)) ? true : false;
}

bool isdigit( int ch ) 
{
	return ( (ch == '0') || (ch == '1') || (ch == '2') || (ch == '3') || (ch == '4') ||
				(ch == '5') || (ch == '6') || (ch == '7') || (ch == '8') || (ch == '9')) ? true : false;
}


long atol( const char* nptr )
{	
	int c;              /* current char */
	long total = 0;         /* current total */
	int sign;           /* if '-', then negative, otherwise positive */
	
	/* skip whitespace */
	while ( BlackBox::isspace((int)(unsigned char)*nptr) )
		++nptr;
	
	c = (int)(unsigned char)*nptr++;
	sign = c;           /* save sign indication */
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*nptr++;    /* skip sign */
	
	total = 0;
	
	while (BlackBox::isdigit(c)) {
		total = 10 * total + (c - '0');     /* accumulate digit */
		c = (int)(unsigned char)*nptr++;    /* get next char */
	}
	
	if (sign == '-')
		return -total;
	else
		return total;   /* return result, negated if necessary */
}


};