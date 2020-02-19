/*
 * File:	ImaIter.h
 * Purpose:	Declaration of the Platform Independent Image Base Class
 * Author:	Alejandro Aguilar Sierra
 * Created:	1995
 * Copyright:	(c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
 *
 * 07/08/2001 Davide Pizzolato - www.xdp.it
 * - removed slow loops
 * - added safe checks
 *
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
 * THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
 * CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
 * THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
 * SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
 * PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.
 *
 * Use at your own risk!
 * ==========================================================
 */

#if !defined(__ImaIter_h)
#define __ImaIter_h

#include "ximage.h"
#include "ximadef.h"

class CImageIterator
{
friend class CxImage;
protected:
	int Itx, Ity;		// Counters
	int Stepx, Stepy;
	unsigned char* IterImage;	//  Image pointer
	CxImage *ima;
public:
	// Constructors
	CImageIterator ( void );
	CImageIterator ( CxImage *image );
	operator CxImage* ();

	// Iterators
	bool ItOK ();
	void Reset ();
	void Upset ();
	void SetRow(unsigned char *buf, int n);
	void GetRow(unsigned char *buf, int n);
	unsigned char GetByte( ) { return IterImage[Itx]; }
	void SetByte(unsigned char b) { IterImage[Itx] = b; }
	unsigned char* GetRow(void);
	unsigned char* GetRow(int n);
	bool NextRow();
	bool PrevRow();
	bool NextByte();
	bool PrevByte();

	void SetSteps(int x, int y=0) {  Stepx = x; Stepy = y; }
	void GetSteps(int *x, int *y) {  *x = Stepx; *y = Stepy; }
	bool NextStep();
	bool PrevStep();

	void SetY(int y);	/* AD - for interlace */
	int  GetY() {return Ity;}
	bool GetCol(unsigned char* pCol, unsigned int x);
	bool SetCol(unsigned char* pCol, unsigned int x);
};

/////////////////////////////////////////////////////////////////////
inline
CImageIterator::CImageIterator(void)
{
	ima = 0;
	IterImage = 0;
	Itx = Ity = 0;
	Stepx = Stepy = 0;
}
/////////////////////////////////////////////////////////////////////
inline
CImageIterator::CImageIterator(CxImage *imageImpl): ima(imageImpl)
{
	if (ima) IterImage = ima->GetBits();
	Itx = Ity = 0;
	Stepx = Stepy = 0;
}
/////////////////////////////////////////////////////////////////////
inline
CImageIterator::operator CxImage* ()
{
	return ima;
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::ItOK ()
{
	if (ima) return ima->IsInside(Itx, Ity);
	else	 return FALSE;
}
/////////////////////////////////////////////////////////////////////
inline void CImageIterator::Reset()
{
	if (ima) IterImage = ima->GetBits();
	else	 IterImage=0;
	Itx = Ity = 0;
}
/////////////////////////////////////////////////////////////////////
inline void CImageIterator::Upset()
{
	Itx = 0;
	Ity = ima->GetHeight()-1;
	IterImage = ima->GetBits() + ima->GetEffWidth()*(ima->GetHeight()-1);
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::NextRow()
{
	if (++Ity >= (int)ima->GetHeight()) return 0;
	IterImage += ima->GetEffWidth();
	return 1;
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::PrevRow()
{
	if (--Ity < 0) return 0;
	IterImage -= ima->GetEffWidth();
	return 1;
}
/* AD - for interlace */
inline void CImageIterator::SetY(int y)
{
	if ((y < 0) || (y > (int)ima->GetHeight())) return;
	Ity = y;
	IterImage = ima->GetBits() + ima->GetEffWidth()*y;
}
/////////////////////////////////////////////////////////////////////
inline void CImageIterator::SetRow(unsigned char *buf, int n)
{
	if (n<0) n = (int)ima->GetEffWidth();
	else n = MIN(n,(int)ima->GetEffWidth());

	if ((IterImage!=NULL)&&(buf!=NULL)&&(n>0)) memcpy(IterImage,buf,n);
}
/////////////////////////////////////////////////////////////////////
inline void CImageIterator::GetRow(unsigned char *buf, int n)
{
	if ((IterImage!=NULL)&&(buf!=NULL)&&(n>0))
		memcpy(buf,IterImage,MIN(n,(int)ima->GetEffWidth()));
}
/////////////////////////////////////////////////////////////////////
inline unsigned char* CImageIterator::GetRow()
{
	return IterImage;
}
/////////////////////////////////////////////////////////////////////
inline unsigned char* CImageIterator::GetRow(int n)
{
	SetY(n);
	return IterImage;
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::NextByte()
{
	if (++Itx < (int)ima->GetEffWidth()) return 1;
	else
		if (++Ity < (int)ima->GetHeight()){
			IterImage += ima->GetEffWidth();
			Itx = 0;
			return 1;
		} else
			return 0;
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::PrevByte()
{
  if (--Itx >= 0) return 1;
  else
	  if (--Ity >= 0){
		  IterImage -= ima->GetEffWidth();
		  Itx = 0;
		  return 1;
	  } else
		  return 0;
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::NextStep()
{
	Itx += Stepx;
	if (Itx < (int)ima->GetEffWidth()) return 1;
	else {
		Ity += Stepy;
		if (Ity < (int)ima->GetHeight()){
			IterImage += ima->GetEffWidth();
			Itx = 0;
			return 1;
		} else
			return 0;
	}
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::PrevStep()
{
	Itx -= Stepx;
	if (Itx >= 0) return 1;
	else {       
		Ity -= Stepy;
		if (Ity >= 0 && Ity < (int)ima->GetHeight()) {
			IterImage -= ima->GetEffWidth();
			Itx = 0;
			return 1;
		} else
			return 0;
	}
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::GetCol(unsigned char* pCol, unsigned int x)
{
	if ((pCol==0)||(ima->GetBpp()<8)||(x>=ima->GetWidth()))
		return 0;
	unsigned int h = ima->GetHeight();
	//unsigned int line = ima->GetEffWidth();
	unsigned char bytes = (unsigned char)(ima->GetBpp()>>3);
	unsigned char* pSrc;
	for (unsigned int y=0;y<h;y++){
		pSrc = ima->GetBits(y) + x*bytes;
		for (unsigned char w=0;w<bytes;w++){
			*pCol++=*pSrc++;
		}
	}
	return 1;
}
/////////////////////////////////////////////////////////////////////
inline bool CImageIterator::SetCol(unsigned char* pCol, unsigned int x)
{
	if ((pCol==0)||(ima->GetBpp()<8)||(x>=ima->GetWidth()))
		return 0;
	unsigned int h = ima->GetHeight();
	//unsigned int line = ima->GetEffWidth();
	unsigned char bytes = (unsigned char)(ima->GetBpp()>>3);
	unsigned char* pSrc;
	for (unsigned int y=0;y<h;y++){
		pSrc = ima->GetBits(y) + x*bytes;
		for (unsigned char w=0;w<bytes;w++){
			*pSrc++=*pCol++;
		}
	}
	return 1;
}
/////////////////////////////////////////////////////////////////////
#endif
