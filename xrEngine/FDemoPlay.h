// CDemoPlay.h: interface for the CDemoPlay class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FDEMOPLAY_H__9B07E6E0_FC3C_11D3_B4E3_4854E82A090D__INCLUDED_)
#define AFX_FDEMOPLAY_H__9B07E6E0_FC3C_11D3_B4E3_4854E82A090D__INCLUDED_
#include "effector.h"

// refs
class	COMotion;
struct	SAnimParams;

// class
class ENGINE_API CDemoPlay :	public CEffectorCam
{
    COMotion*			m_pMotion			;
    SAnimParams*		m_MParam			;

	xr_vector<Fmatrix>	seq					;
	int					m_count				;
	float				fStartTime			;
	float				fSpeed				;
	u32					dwCyclesLeft		;

	// statistics
	BOOL				stat_started		;
	CTimer				stat_Timer_frame	;
	CTimer				stat_Timer_total	;
	u32					stat_StartFrame		;
	xr_vector<float>	stat_table			;
	
	void				stat_Start	()		;
	void				stat_Stop	()		;
public:
	virtual BOOL		ProcessCam		(SCamEffectorInfo& info);

						CDemoPlay	(const char *name, float ms, u32 cycles, float life_time=60*60*1000);
	virtual				~CDemoPlay	();
};

#endif // !defined(AFX_FDEMOPLAY_H__9B07E6E0_FC3C_11D3_B4E3_4854E82A090D__INCLUDED_)
