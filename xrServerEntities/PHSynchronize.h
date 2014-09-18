#ifndef PH_SYNCHRONIZE_H
#define PH_SYNCHRONIZE_H

#include "../xrserverentities/PHNetState.h"

class NET_Packet;

class CPHSynchronize
{
public:
	virtual void						net_Export			(		NET_Packet&		P)							{};					// export to server
	virtual void						net_Import			(		NET_Packet&		P)							{};
	virtual void						get_State			(		SPHNetState&	state)								=0;
	virtual void						set_State			(const	SPHNetState&	state)								=0;
	virtual void						cv2obj_Xfrom		(const Fquaternion& q,const Fvector& pos, Fmatrix& xform)	=0;
	virtual void						cv2bone_Xfrom		(const Fquaternion& q,const Fvector& pos, Fmatrix& xform)	=0;
protected:
private:
};



#endif