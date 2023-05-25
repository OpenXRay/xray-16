// Image.h: interface for the CImage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGE_H__4281EEAB_9522_40E5_A90D_831A40E1A344__INCLUDED_)
#define AFX_IMAGE_H__4281EEAB_9522_40E5_A90D_831A40E1A344__INCLUDED_
#pragma once

class ENGINE_API CImage
{
public:
	u32 dwWidth;
	u32 dwHeight;
	BOOL bAlpha;
	u32 *pData;

	CImage() { ZeroMemory(this, sizeof(*this)); }
	~CImage() { xr_free(pData); }

	void Create(u32 w, u32 h);
	void Create(u32 w, u32 h, u32 *data);
	void Load(LPCSTR name);
	bool LoadTGA(LPCSTR name);
	void SaveTGA(LPCSTR name, BOOL b24 = FALSE);

	void Vflip(void);
	void Hflip(void);
	void Contrast(float Q);
	void Grayscale();

	__forceinline u32 GetPixel(int x, int y) { return pData[y * dwWidth + x]; }
	__forceinline void PutPixel(int x, int y, u32 p) { pData[y * dwWidth + x] = p; }
};

#endif // !defined(AFX_IMAGE_H__4281EEAB_9522_40E5_A90D_831A40E1A344__INCLUDED_)
