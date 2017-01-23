// MusicStream.h: interface for the CMusicStream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MUSICSTREAM_H__7DAD65D5_8E32_4262_89C8_67A135405BAF__INCLUDED_)
#define AFX_MUSICSTREAM_H__7DAD65D5_8E32_4262_89C8_67A135405BAF__INCLUDED_
#pragma once

// refs
class ENGINE_API CSoundStream;
class ENGINE_API CInifile;

class CMusicStream {
	xr_vector<CSoundStream*>	streams;
	int							FindEmptySlot();
public:
							CMusicStream	();
							~CMusicStream	();

	CSoundStream*			CreateSound		(LPCSTR name	);
	void					DeleteSound		(CSoundStream* pSnd);

	void					OnMove			();
	void					Reload			();
	void					Update			();
};

#endif // !defined(AFX_MUSICSTREAM_H__7DAD65D5_8E32_4262_89C8_67A135405BAF__INCLUDED_)
