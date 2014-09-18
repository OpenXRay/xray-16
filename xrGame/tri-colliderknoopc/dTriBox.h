#ifndef		D_TRI_BOX_H
#define		D_TRI_BOX_H

#include "TriPrimitiveCollideClassDef.h"
#include "../ode_include.h"
#include "../MathUtils.h"
#include "dcTriListCollider.h"
struct Triangle;
struct dxBox {
	dVector3 side;	// side lengths (x,y,z)
};

IC float	dcTriListCollider::	dBoxProj(dxGeom* box,const dReal* normal)
{
	VERIFY (dGeomGetClass(box)== dBoxClass);
	float hside[3];
	dGeomBoxGetLengths(box,hside);
	hside[0]*=.5f;hside[1]*=0.5f;hside[2]*=0.5f;
	const dReal* R=dGeomGetRotation(box);
	return
		dFabs(dDOT14(normal,R+0)*hside[0])+
		dFabs(dDOT14(normal,R+1)*hside[1])+
		dFabs(dDOT14(normal,R+2)*hside[2]);
}



IC void dcTriListCollider::CrossProjLine(const dReal* pt1,const dReal* vc1,const dReal* pt2,const dReal* vc2,dReal* proj){
	dVector3 ac={pt1[0]-pt2[0],pt1[1]-pt2[1],pt1[2]-pt2[2]};
	dReal factor=(dDOT(vc2,vc2)*dDOT44(vc1,vc1)-dDOT14(vc2,vc1)*dDOT14(vc2,vc1));
	if(factor==0.f){
		proj[0]=dInfinity;
		//proj[1]=dInfinity;
		//proj[2]=dInfinity;
		return;
	}
	dReal t=(dDOT(ac,vc2)*dDOT41(vc1,vc2)-dDOT41(vc1,ac)*dDOT(vc2,vc2))
		/
		factor;

	proj[0]=pt1[0]+vc1[0]*t;
	proj[1]=pt1[1]+vc1[4]*t;
	proj[2]=pt1[2]+vc1[8]*t;


}

IC void dcTriListCollider::CrossProjLine1(const dReal* pt1,const dReal* vc1,const dReal* pt2,const dReal* vc2,dReal* proj){
	dVector3 ac={pt1[0]-pt2[0],pt1[1]-pt2[1],pt1[2]-pt2[2]};
	dReal factor=(dDOT44(vc2,vc2)*dDOT(vc1,vc1)-dDOT41(vc2,vc1)*dDOT41(vc2,vc1));
	if(factor==0.f){
		proj[0]=dInfinity;
		//proj[1]=dInfinity;
		//proj[2]=dInfinity;
		return;
	}
	dReal t=(dDOT14(ac,vc2)*dDOT14(vc1,vc2)-dDOT(vc1,ac)*dDOT44(vc2,vc2))
		/
		factor;

	proj[0]=pt1[0]+vc1[0]*t;
	proj[1]=pt1[1]+vc1[1]*t;
	proj[2]=pt1[2]+vc1[2]*t;


}


IC bool	dcTriListCollider:: CrossProjLine14(const dReal* pt1,const dReal* vc1,const dReal* pt2,const dReal* vc2,dReal hside,dReal* proj){
	dVector3 ac={pt1[0]-pt2[0],pt1[1]-pt2[1],pt1[2]-pt2[2]};

	//dReal vc2_2=dDOT44(vc2,vc2);
	dReal vc1_vc2=dDOT41(vc2,vc1);
	dReal vc1_2=dDOT(vc1,vc1);

	dReal factor=/*vc2_2*/vc1_2-vc1_vc2*vc1_vc2;
	if(factor==0.f){
		//proj[0]=dInfinity;
		//proj[1]=dInfinity;
		//proj[2]=dInfinity;
		return false;
	}
	dReal ac_vc1=dDOT(vc1,ac);
	dReal ac_vc2=dDOT14(ac,vc2);
	dReal t1=(ac_vc2*vc1_vc2-ac_vc1/*vc2_2*/)
		/
		factor;

	if(t1<0.f) return false;
	if(t1>1.f) return false;

	dReal t2=(ac_vc1*vc1_vc2-ac_vc2*vc1_2)
		/factor;

	dReal nt2=t2;//*_sqrt(vc2_2);
	if(nt2>hside || nt2 < -hside) return false;

	proj[0]=pt1[0]+vc1[0]*t1;
	proj[1]=pt1[1]+vc1[1]*t1;
	proj[2]=pt1[2]+vc1[2]*t1;

	return true;
}
//is point in Box
IC bool dcTriListCollider::IsPtInBx(const dReal* Pt,const dReal* BxP,const dReal* BxEx,const dReal* BxR){
	dVector3 BxPR,PtR;

	dMULTIPLY1_331 (BxPR,BxR,BxP);
	dMULTIPLY1_331 (PtR,BxR,Pt);
	return

		dFabs(BxPR[0]-PtR[0])<BxEx[0]/2&&
		dFabs(BxPR[1]-PtR[1])<BxEx[1]/2&&
		dFabs(BxPR[2]-PtR[2])<BxEx[2]/2;
}

inline dReal PointBoxTest(const dReal* Pt,const dReal* BxP,const dReal* BxEx,const dReal* R,dReal* norm){
	dVector3 BxPR,PtR; 
	dVector3 normR={0.f,0.f,0.f};

	dMULTIPLY1_331 (BxPR,R,BxP);
	dMULTIPLY1_331 (PtR,R,Pt);
	dReal depth0,depth1,depth2;

	depth0=-dFabs(BxPR[0]-PtR[0])+BxEx[0]/2;
	if(depth0<0) return -1.f;

	depth1=-dFabs(BxPR[1]-PtR[1])+BxEx[1]/2;
	if(depth1<0) return -1.f;

	depth2=-dFabs(BxPR[2]-PtR[2])+BxEx[2]/2;
	if(depth2<0) return -1;



	if(depth0<depth1){

		if(depth0<depth2)	{
			normR[0]=PtR[0]-BxPR[0];
			dMULTIPLY0_331(norm,R,normR);
			return depth0;
		}

		else				{
			normR[2]=PtR[2]-BxPR[2];
			dMULTIPLY0_331(norm,R,normR);
			return depth2;
		}
	}
	else{

		if(depth1<depth2)	{ 
			normR[1]=PtR[1]-BxPR[1];
			dMULTIPLY0_331(norm,R,normR);
			return depth1;
		}
		else				{
			normR[2]=PtR[2]-BxPR[2];
			dMULTIPLY0_331(norm,R,normR);
			return   depth2;
		}
	}

}

IC dReal dcTriListCollider:: FragmentonBoxTest(const dReal* Pt1,const dReal* Pt2,const dReal* BxP,const dReal* BxEx,const dReal* R,dReal* norm,dReal* pos){

	dVector3 fragmentonAx={Pt2[0]-Pt1[0],Pt2[1]-Pt1[1],Pt2[2]-Pt1[2]};
	dReal BxExPr;
	accurate_normalize(fragmentonAx);
	dReal BxPPr=dDOT(fragmentonAx,BxP);
	BxExPr=
		dFabs(dDOT14(fragmentonAx,R+0)*BxEx[0])+
		dFabs(dDOT14(fragmentonAx,R+1)*BxEx[1])+
		dFabs(dDOT14(fragmentonAx,R+2)*BxEx[2]);

	if(
		(dDOT(fragmentonAx,Pt1)-BxPPr-BxExPr/2.f)*
		(dDOT(fragmentonAx,Pt2)-BxPPr-BxExPr/2.f)>0.f
		&&
		(dDOT(fragmentonAx,Pt1)-BxPPr+BxExPr/2.f)*
		(dDOT(fragmentonAx,Pt2)-BxPPr+BxExPr/2.f)>0.f	
		) return -1.f;


	dVector3 crossAx0;
	dCROSS114(crossAx0,=,fragmentonAx,R+0);
	accurate_normalize(crossAx0);	
	BxExPr=
		dFabs(dDOT14(crossAx0,R+0)*BxEx[0])+
		dFabs(dDOT14(crossAx0,R+1)*BxEx[1])+
		dFabs(dDOT14(crossAx0,R+2)*BxEx[2]);
	dReal distance0=dDOT(crossAx0,Pt1)-dDOT(crossAx0,BxP);
	if(dFabs(distance0)>BxExPr/2.f) return -1.f;
	dReal depth0=BxExPr/2.f-dFabs(distance0);

	dVector3 crossAx1;
	dCROSS114(crossAx1,=,fragmentonAx,R+1);
	accurate_normalize(crossAx1);
	BxExPr=
		dFabs(dDOT14(crossAx1,R+0)*BxEx[0])+
		dFabs(dDOT14(crossAx1,R+1)*BxEx[1])+
		dFabs(dDOT14(crossAx1,R+2)*BxEx[2]);
	dReal distance1=dDOT(crossAx1,Pt1)-dDOT(crossAx1,BxP);
	if(dFabs(distance1)>BxExPr/2.f) return -1.f;
	dReal depth1=BxExPr/2.f-dFabs(distance1);

	dVector3 crossAx2;
	dCROSS114(crossAx2,=,fragmentonAx,R+2);
	accurate_normalize(crossAx2);
	BxExPr=
		dFabs(dDOT14(crossAx2,R+0)*BxEx[0])+
		dFabs(dDOT14(crossAx2,R+1)*BxEx[1])+
		dFabs(dDOT14(crossAx2,R+2)*BxEx[2]);
	dReal distance2=dDOT(crossAx2,Pt1)-dDOT(crossAx2,BxP);
	if(dFabs(distance2)>BxExPr/2.f) return -1.f;
	dReal depth2=BxExPr/2.f-dFabs(distance2);


	if(depth0<depth1){

		if(depth0<depth2)						{
			norm[0]=distance0*crossAx0[0];
			norm[1]=distance0*crossAx0[1];
			norm[2]=distance0*crossAx0[2];
			CrossProjLine(BxP,R+0,Pt1,fragmentonAx,pos);
			pos[0]+=norm[0];
			pos[1]+=norm[1];
			pos[2]+=norm[2];
			return depth0;
		}

		else									{

			norm[0]=distance2*crossAx2[0];
			norm[1]=distance2*crossAx2[1];
			norm[2]=distance2*crossAx2[2];
			CrossProjLine(BxP,R+2,Pt1,fragmentonAx,pos);
			pos[0]+=norm[0];
			pos[1]+=norm[1];
			pos[2]+=norm[2];
			return depth2;
		}
	}


	else{

		if(depth1<depth2)						{ 
			norm[0]=distance1*crossAx1[0];
			norm[1]=distance1*crossAx1[1];
			norm[2]=distance1*crossAx1[2];
			CrossProjLine(BxP,R+1,Pt1,fragmentonAx,pos);
			pos[0]+=norm[0];
			pos[1]+=norm[1];
			pos[2]+=norm[2];
			return depth1;
		}
		else 									{
			norm[0]=distance2*crossAx2[0];
			norm[1]=distance2*crossAx2[1];
			norm[2]=distance2*crossAx2[2];
			CrossProjLine(BxP,R+2,Pt1,fragmentonAx,pos);
			pos[0]+=norm[0];
			pos[1]+=norm[1];
			pos[2]+=norm[2];
			return depth2;
		}
	}

}

/*
inline dReal FragmentonBoxTest1(const dReal* Pt1,const dReal* Pt2,const dReal* BxP,const dReal* BxEx,const dReal* R,dReal* norm){

dVector3 fragmentonAx={Pt2[0]-Pt1[0],Pt2[1]-Pt1[1],Pt2[2]-Pt1[2]};
dVector3 currentAx;
dReal* axis;
dReal currentDepth,depth;
dReal currentDistance,distanse;
dReal BxExPr;
accurate_normalize(fragmentonAx);
dReal BxPPr=dDOT(fragmentonAx,BxP);

BxExPr=
dFabs(dDOT14(fragmentonAx,R+0)*BxEx[0])+
dFabs(dDOT14(fragmentonAx,R+1)*BxEx[1])+
dFabs(dDOT14(fragmentonAx,R+2)*BxEx[2]);

currentDistance=dDOT(fragmentonAx,Pt1)-BxPPr;
currentDepth=BxExPr-dFabs(currentDistance);

distance=dDOT(fragmentonAx,Pt2)-BxPPr;
depth=BxExPr-dFabs(distance);

if(
(currentDistance-BxExPr/2.f)*
(distance-BxExPr/2.f)>0.f
&&
(currentDistance+BxExPr/2.f)*
(distance+BxExPr/2.f)>0.f	
) return -1.f;

if(depth<0.f&&currentDepth>=0.f) {
depth=currentDepth;
distance=currentDistance;
}
else 
dVector3 crossAx0;
dCROSS114(crossAx0,=,fragmentonAx,R+0);
accurate_normalize(crossAx0);	
BxExPr=
dFabs(dDOT14(crossAx0,R+0)*BxEx[0])+
dFabs(dDOT14(crossAx0,R+1)*BxEx[1])+
dFabs(dDOT14(crossAx0,R+2)*BxEx[2]);
dReal distance0=dDOT(crossAx0,Pt1)-dDOT(crossAx0,BxP);
if(dFabs(distance0)>BxExPr/2.f) return -1.f;
dReal depth0=BxExPr/2.f-dFabs(distance0);

dVector3 crossAx1;
dCROSS114(crossAx1,=,fragmentonAx,R+1);
accurate_normalize(crossAx1);
BxExPr=
dFabs(dDOT14(crossAx1,R+0)*BxEx[0])+
dFabs(dDOT14(crossAx1,R+1)*BxEx[1])+
dFabs(dDOT14(crossAx1,R+2)*BxEx[2]);
dReal distance1=dDOT(crossAx1,Pt1)-dDOT(crossAx1,BxP);
if(dFabs(distance1)>BxExPr/2.f) return -1.f;
dReal depth1=BxExPr/2.f-dFabs(distance1);

dVector3 crossAx2;
dCROSS114(crossAx2,=,fragmentonAx,R+2);
accurate_normalize(crossAx2);
BxExPr=
dFabs(dDOT14(crossAx2,R+0)*BxEx[0])+
dFabs(dDOT14(crossAx2,R+1)*BxEx[1])+
dFabs(dDOT14(crossAx2,R+2)*BxEx[2]);
dReal distance2=dDOT(crossAx2,Pt1)-dDOT(crossAx2,BxP);
if(dFabs(distance2)>BxExPr/2.f) return -1.f;
dReal depth2=BxExPr/2.f-dFabs(distance2);


if(depth0<depth1){

if(depth0<depth2)						{
norm[0]=distance0*crossAx0[0];
norm[1]=distance0*crossAx0[1];
norm[2]=distance0*crossAx0[2];
return depth0;
}

else									{

norm[0]=distance2*crossAx2[0];
norm[1]=distance2*crossAx2[1];
norm[2]=distance2*crossAx2[2];
return depth2;
}
}


else{

if(depth1<depth2)						{ 
norm[0]=distance1*crossAx1[0];
norm[1]=distance1*crossAx1[1];
norm[2]=distance1*crossAx1[2];
return depth1;
}
else 									{
norm[0]=distance2*crossAx2[0];
norm[1]=distance2*crossAx2[1];
norm[2]=distance2*crossAx2[2];
return depth2;
}
}

}
*/



#endif