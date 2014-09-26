#pragma once

#include "customzone.h"
#include "script_export_space.h"

class CMosquitoBald : public CCustomZone
{
private:
	typedef	CCustomZone	inherited;
public:
					CMosquitoBald				();
	virtual			~CMosquitoBald				();

	virtual void	Load						(LPCSTR section);

	virtual void	Affect						(SZoneObjectInfo* O);

protected:
	virtual bool	BlowoutState				();
	virtual	void	UpdateSecondaryHit			();
	//для того чтобы blowout обновился один раз
	//после того как зона перключилась в другое состояние
	bool			m_bLastBlowoutUpdate;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CMosquitoBald)
#undef script_type_list
#define script_type_list save_type_list(CMosquitoBald)