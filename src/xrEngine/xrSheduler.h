#ifndef XRSHEDULER_H_INCLUDED
#define XRSHEDULER_H_INCLUDED

#include "ISheduled.h"

class	ENGINE_API	CSheduler
{
private:
	struct Item
	{
		u32			dwTimeForExecute;
		u32			dwTimeOfLastExecute;
		shared_str	scheduled_name;
		ISheduled*	Object;
		u32			dwPadding;				// for align-issues

		IC bool		operator < (Item& I)
		{	return dwTimeForExecute > I.dwTimeForExecute; }
	};
	struct	ItemReg
	{
		BOOL		OP;
		BOOL		RT;
		ISheduled*	Object;
	};
private:
	xr_vector<Item>			ItemsRT			;
	xr_vector<Item>			Items			;
	xr_vector<Item>			ItemsProcessed	;
	xr_vector<ItemReg>		Registration	;
	ISheduled*				m_current_step_obj;
	bool					m_processing_now;

	IC void			Push	(Item& I);
	IC void			Pop		();
	IC Item&		Top		()
	{
		return Items.front();
	}
	void			internal_Register		(ISheduled* A, BOOL RT=FALSE		);
	bool			internal_Unregister		(ISheduled* A, BOOL RT, bool warn_on_not_found = true);
	void			internal_Registration	();
public:
	u64				cycles_start;
	u64				cycles_limit;
public:
	void			ProcessStep	();
	void			Process		();
	void			Update		();

#ifdef DEBUG
	bool			Registered	(ISheduled *object) const;
#endif // DEBUG
	void			Register	(ISheduled* A, BOOL RT=FALSE		);
	void			Unregister	(ISheduled* A						);
	void			EnsureOrder	(ISheduled* Before, ISheduled* After);

	void			Initialize	();
	void			Destroy		();
};

#endif // XRSHEDULER_H_INCLUDED
