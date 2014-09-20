#ifndef BaseH
#define BaseH

//#include <xrCore.h>
#include "PostprocessAnimator.h"
//#include "single_param.h"

enum{tAddColor=0,tBaseColor=1,tGray=2,tDuality=3,tNoise=4,tBlur=5,tColorMap=6};

class TPPPropEditor
{
public:
    virtual TForm*	GetForm				() 				=0;
	virtual void	Clear 				() 				=0;
    virtual void	ShowCurrent			(u32 keyIdx) 	=0;
    virtual _pp_params GetTimeChannel	() 				=0;
    virtual bool	DrawChannel			(_pp_params) 	=0;
    virtual void	Lock				(bool b) 		=0;
    virtual void	AddNew				(u32 keyIdx) 	=0;
    virtual void	Remove				(u32 keyIdx) 	=0;
    virtual void	RemoveAllKeys		()				=0;
    virtual void	CreateKey			(float t) 		=0;
};


#endif
