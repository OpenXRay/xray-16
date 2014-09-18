#ifndef GLOBAL_FEEL_TOUCH_HPP
#define GLOBAL_FEEL_TOUCH_HPP

#include "../xrEngine/feel_touch.h"

//this class implements only denie functionality
class GlobalFeelTouch : public Feel::Touch
{
public:
							GlobalFeelTouch();
	virtual					~GlobalFeelTouch();

	virtual void			feel_touch_update			(Fvector& P, float	R);
	//virtual void			feel_touch_deny				(CObject* O, DWORD	T); - implemented in inherited class

			bool			is_object_denied			(CObject const * O);
}; //class GlobalFeelTouch

#endif //#ifndef GLOBAL_FEEL_TOUCH_HPP