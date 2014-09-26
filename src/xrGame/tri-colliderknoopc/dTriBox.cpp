#include "stdafx.h"
#include "dTriColliderCommon.h"
#include "dTriBox.h"
#include "dcTriListCollider.h"


int dcTriListCollider::dSortedTriBox (
						const dReal* triSideAx0,const dReal* triSideAx1,
						const dReal* triAx,
						//const dReal* v0,
						//const dReal* v1,
						//const dReal* v2,
						CDB::TRI* T,
						dReal dist,
						dxGeom *o1, dxGeom *o2,
						int flags, dContactGeom *contact, int skip
						)
{



  VERIFY (skip >= (int)sizeof(dContactGeom));
  VERIFY (dGeomGetClass(o1) == dBoxClass);
  

  
  const dReal *R = dGeomGetRotation(o1);
  const dReal* p=dGeomGetPosition(o1);
  dVector3 hside;
  dGeomBoxGetLengths(o1,hside);
  hside[0]/=2.f;hside[1]/=2.f;hside[2]/=2.f;
    // find number of contacts requested
  int maxc = flags & NUMC_MASK;
  if (maxc < 1) maxc = 1;
  if (maxc > 3) maxc = 3;	// no more than 3 contacts per box allowed

  int code=0;
  dReal outDepth;
  char signum;//,sn;


 dReal sidePr=
	dFabs(dDOT14(triAx,R+0)*hside[0])+
	dFabs(dDOT14(triAx,R+1)*hside[1])+
	dFabs(dDOT14(triAx,R+2)*hside[2]);


dReal depth=sidePr-dist;//dFabs(dist);
outDepth=depth;
signum=-1;//dist<0.f ? -1 : 1;
code=0;

if(depth<0.f) return 0;

unsigned int i;

dVector3 norm,pos;
unsigned int ret=1;

if(code==0){
	norm[0]=triAx[0]*signum;
	norm[1]=triAx[1]*signum;
	norm[2]=triAx[2]*signum;

/////////////////////////////////////////// from geom.cpp dCollideBP
  dReal Q1 = -signum*dDOT14(triAx,R+0);
  dReal Q2 = -signum*dDOT14(triAx,R+1);
  dReal Q3 = -signum*dDOT14(triAx,R+2);
  dReal A1 = 2.f*hside[0] * Q1;
  dReal A2 = 2.f*hside[1] * Q2;
  dReal A3 = 2.f*hside[2] * Q3;
  dReal B1 = dFabs(A1);
  dReal B2 = dFabs(A2);
  dReal B3 = dFabs(A3);

  pos[0]=p[0];
  pos[1]=p[1];
  pos[2]=p[2];

#define FOO(i,op) \
  pos[0] op hside[i] * R[0+i]; \
  pos[1] op hside[i] * R[4+i]; \
  pos[2] op hside[i] * R[8+i];
#define BAR(i,iinc) if (A ## iinc > 0) { FOO(i,-=) } else { FOO(i,+=) }
  BAR(0,1);
  BAR(1,2);
  BAR(2,3);
#undef FOO
#undef BAR

///////////////////////////////////////////////////////////


if (maxc == 1) goto done;

  // get the second and third contact points by starting from `p' and going
  // along the two sides with the smallest projected length.

//(@slipch) it is not perfectly right for triangle collision 
//because it need to check if additional points are in the triangle but it seems cause no problem
 
#define FOO(i,j,op) \
  CONTACT(contact,i*skip)->pos[0] = pos[0] op 2.f*hside[j] * R[0+j]; \
  CONTACT(contact,i*skip)->pos[1] = pos[1] op 2.f*hside[j] * R[4+j]; \
  CONTACT(contact,i*skip)->pos[2] = pos[2] op 2.f*hside[j] * R[8+j];
#define BAR(ctact,side,sideinc) \
  depth -= B ## sideinc; \
  if (depth < 0) goto done; \
  if (A ## sideinc > 0) { FOO(ctact,side,+) } else { FOO(ctact,side,-) } \
  CONTACT(contact,ctact*skip)->depth = depth; \
  ++ret;

  CONTACT(contact,skip)->normal[0] =  triAx[0]*signum;
  CONTACT(contact,skip)->normal[1] =  triAx[1]*signum;
  CONTACT(contact,skip)->normal[2] =  triAx[2]*signum;
  if (maxc == 3) {
    CONTACT(contact,2*skip)->normal[0] = triAx[0]*signum;
    CONTACT(contact,2*skip)->normal[1] = triAx[1]*signum;
    CONTACT(contact,2*skip)->normal[2] = triAx[2]*signum;
  }

  if (B1 < B2) {
    if (B3 < B1) goto use_side_3; else {
      BAR(1,0,1);	// use side 1
      if (maxc == 2) goto done;
      if (B2 < B3) goto contact2_2; else goto contact2_3;
    }
  }
  else {
    if (B3 < B2) {
      use_side_3:	// use side 3
      BAR(1,2,3);
      if (maxc == 2) goto done;
      if (B1 < B2) goto contact2_1; else goto contact2_2;
    }
    else {
      BAR(1,1,2);	// use side 2
      if (maxc == 2) goto done;
      if (B1 < B3) goto contact2_1; else goto contact2_3;
    }
  }

  contact2_1: BAR(2,0,1); goto done;
  contact2_2: BAR(2,1,2); goto done;
  contact2_3: BAR(2,2,3); goto done;
#undef FOO
#undef BAR

 done: ;

////////////////////////////////////////////////////////////// end (from geom.cpp dCollideBP)
  
	}


contact->pos[0] = pos[0];
contact->pos[1] = pos[1];
contact->pos[2] = pos[2];

contact->depth = outDepth;

 for (i=0; i<ret; ++i) {
    CONTACT(contact,i*skip)->g1 = const_cast<dxGeom*> (o2);
    CONTACT(contact,i*skip)->g2 = const_cast<dxGeom*> (o1);
	CONTACT(contact,i*skip)->normal[0] = norm[0];
	CONTACT(contact,i*skip)->normal[1] = norm[1];
	CONTACT(contact,i*skip)->normal[2] = norm[2];
	SURFACE(contact,i*skip)->mode=T->material;
  }
  if(ret&&dGeomGetUserData(o1)->callback)dGeomGetUserData(o1)->callback(T,contact);
  return ret;

}

/*
bool test_cross_side( dReal* outAx, dReal& outDepth, dReal *pos, dReal& outSignum, int &code, u8 c,  const dReal* R, const dReal* hside, const dReal* p,  const dReal* triSideAx, const dReal* vax,   const dReal* vox  )
{

for( u8 i=0;i<3;++i){
	dVector3 axis;
	dCROSS114(axis,=,triSideAx ,R+i );
	accurate_normalize(axis);
	int ix1=(i+1)%3;
	int ix2=(i+2)%3;
	dReal sidePr=
		dFabs(dDOT14(axis,R+ix1)*hside[ix1])+
		dFabs(dDOT14(axis,R+ix2)*hside[ix2]);

	dReal	dist_ax=dDOT(vax,axis)-dDOT(p,axis);
	dReal	dist_ox=dDOT(vox,axis)-dDOT(p,axis);

	bool isPdistax=dist_ax>0.f;
	bool isPdistox=dist_ox>0.f;
	if(isPdistax != isPdistox) continue;

	dReal depth_ax=sidePr-dFabs(dist_ax);
	dReal depth_ox=sidePr-dFabs(dist_ox);
	if( depth_ax>depth_ox ){
		if(depth_ax>0.f){
			if(depth_ax*1.05f<outDepth) 
						{
							dReal sgn=dist_ax<0.f ? -1.f : 1.f;
							dReal sgn1=sgn*dDOT14(axis,R+ix1)<0.f ? -1.f : 1.f;
							dReal sgn2=sgn*dDOT14(axis,R+ix2)<0.f ? -1.f : 1.f;
							dVector3 crpos;
							for(int ii=0;ii<3;++ii)
								crpos[ii]=p[ii]+R[ii*4+ix1]*hside[ix1]*sgn1+R[ii*4+ix2]*hside[ix2]*sgn2;
						
							if( dcTriListCollider::CrossProjLine14( vax, triSideAx, crpos,R+i, hside[i], pos ) )
							{
							outDepth=depth_ax;
							outSignum=sgn;
							outAx[0]=axis[0];
							outAx[1]=axis[1];
							outAx[2]=axis[2];
							code=c+i;
							}
						}
					}
			else return true;
		}
	}
 return false;
}
*/
IC bool normalize_if_possible( dReal *v )
{
	dReal sqr_magnitude = dDOT(v,v);
	if( sqr_magnitude < EPS_S )
		return false;
	dReal	l	=	dRecipSqrt(sqr_magnitude);
		v[0]		*=	l;
		v[1]		*=	l;
		v[2]		*=	l;
	return true;
}

int dcTriListCollider::dTriBox (
						const dReal* v0,const dReal* v1,const dReal* v2,
						Triangle* T,
						dxGeom *o1, dxGeom *o2,
						int flags, dContactGeom *contact, int skip
						)
{



  VERIFY (skip >= (int)sizeof(dContactGeom));
  VERIFY (dGeomGetClass(o1) == dBoxClass);
  
  
  
  const dReal *R = dGeomGetRotation(o1);
  const dReal* p=dGeomGetPosition(o1);
  dVector3 hside;
  dGeomBoxGetLengths(o1,hside);
  hside[0]/=2.f;hside[1]/=2.f;hside[2]/=2.f;

    // find number of contacts requested
  int maxc = flags & NUMC_MASK;
  if (maxc < 1) maxc = 1;
  if (maxc > 3) maxc = 3;	// no more than 3 contacts per box allowed

  //dVector3 triAx;
  const dReal* triSideAx0=T->side0;//{v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
  const dReal* triSideAx1=T->side1;//{v2[0]-v1[0],v2[1]-v1[1],v2[2]-v1[2]};
  dVector3 triSideAx2={v0[0]-v2[0],v0[1]-v2[1],v0[2]-v2[2]};
  //dCROSS(triAx,=,triSideAx0,triSideAx1);
  int code=0;
  dReal outDepth;
  dReal signum;
  //sepparation along tri plane normal;
  const dReal *triAx	=T->norm;
 //accurate_normalize(triAx);


 dReal sidePr=
	dFabs(dDOT14(triAx,R+0)*hside[0])+
	dFabs(dDOT14(triAx,R+1)*hside[1])+
	dFabs(dDOT14(triAx,R+2)*hside[2]);

dReal dist=-T->dist;
//dist=dDOT(triAx,v0)-dDOT(triAx,p);
dReal depth=sidePr-dFabs(dist);
outDepth=depth;
signum=dist<0.f ? -1.f : 1.f;
code=0;
if(depth<0.f) return 0;


bool isPdist0,isPdist1,isPdist2;
bool test0=true,test1=true,test2=true;
bool test00,test01,test02;
bool test10,test11,test12;
bool test20,test21,test22;

dReal depth0,depth1,depth2;
dReal dist0,dist1,dist2;



#define CMP(sd,c)	\
if(depth0>depth1)\
		if(depth0>depth2) \
			if(test0##sd){\
			  if(test0)\
				if(depth0<outDepth)\
					{\
					outDepth=depth0;\
					signum=dist0<0.f ? -1.f : 1.f;\
					code=c;\
					}\
			}\
			else return 0;\
		else\
			if(test2##sd){\
			  if(test2)\
				if(depth2<outDepth) \
					{\
					outDepth=depth2;\
					signum=dist2<0.f ? -1.f : 1.f;\
					code=c+2;\
					}\
			}\
			else return 0;\
else\
		if(depth1>depth2)\
			if(test1##sd){\
			  if(test1)\
				if(depth1<outDepth) \
					{\
					outDepth=depth1;\
					signum=dist1<0.f ? -1.f : 1.f;\
					code=c+1;\
					}\
			}\
			else return 0;\
\
		else\
			if(test2##sd){\
			  if(test2)\
				if(depth2<outDepth) \
					{\
					outDepth=depth2;\
					signum=dist2<0.f ? -1.f : 1.f;\
					code=c+2;\
					}\
			}\
			else return 0;


#define TEST(sd, c) \
\
dist0=dDOT14(v0,R+sd)-dDOT14(p,R+sd);\
dist1=dDOT14(v1,R+sd)-dDOT14(p,R+sd);\
dist2=dDOT14(v2,R+sd)-dDOT14(p,R+sd);\
\
isPdist0=dist0>0.f;\
isPdist1=dist1>0.f;\
isPdist2=dist2>0.f;\
\
depth0=hside[sd]-dFabs(dist0);\
depth1=hside[sd]-dFabs(dist1);\
depth2=hside[sd]-dFabs(dist2);\
test0##sd = depth0>0.f;\
test1##sd = depth1>0.f;\
test2##sd = depth2>0.f;\
\
test0 =test0 && test0##sd;\
test1 =test1 && test1##sd;\
test2 =test2 && test2##sd;\
\
if(isPdist0==isPdist1 && isPdist1==isPdist2)\
{\
CMP(sd,c)\
}

TEST(0,1)
TEST(1,4)
TEST(2,7)

#undef CMP
#undef TEST

unsigned int i;

dVector3 axis,outAx;

/*
#define TEST(ax,ox,c) \
for(i=0;i<3;++i){\
	dCROSS114(axis,=,triSideAx##ax,R+i);\
	accurate_normalize(axis);\
	int ix1=(i+1)%3;\
	int ix2=(i+2)%3;\
	sidePr=\
		dFabs(dDOT14(axis,R+ix1)*hside[ix1])+\
		dFabs(dDOT14(axis,R+ix2)*hside[ix2]);\
\
	dist##ax=(dDOT(v##ax,axis)-dDOT(p,axis));\
	dist##ox=(dDOT(v##ox,axis)-dDOT(p,axis));\
			signum=dist##ox<0.f ? -1.f : 1.f;\
\
depth##ax=sidePr-signum*dist##ax;\
depth##ox=sidePr-signum*dist##ox;\
	if(depth##ax<depth##ox){\
			if(depth##ax>0.f){\
			if(depth##ax<outDepth){ \
				if(dDOT(axis,triAx)*signum<0.f)	{\
						outDepth=depth##ax;\
						outAx[0]=axis[0];\
						outAx[1]=axis[1];\
						outAx[2]=axis[2];\
						code=c+i;\
					}\
			}\
				}\
			else return 0;\
	}\
}
*/
dVector3 pos;

#define TEST(ax,ox,c) \
for(i=0;i<3;++i){\
	dCROSS114(axis,=,triSideAx##ax,R+i);\
	if(!normalize_if_possible(axis)) continue;\
	int ix1=(i+1)%3;\
	int ix2=(i+2)%3;\
	sidePr=\
		dFabs(dDOT14(axis,R+ix1)*hside[ix1])+\
		dFabs(dDOT14(axis,R+ix2)*hside[ix2]);\
\
	dist##ax=dDOT(v##ax,axis)-dDOT(p,axis);\
	dist##ox=dDOT(v##ox,axis)-dDOT(p,axis);\
\
isPdist##ax=dist##ax>0.f;\
isPdist##ox=dist##ox>0.f;\
if(isPdist##ax != isPdist##ox) continue;\
\
depth##ax=sidePr-dFabs(dist##ax);\
depth##ox=sidePr-dFabs(dist##ox);\
	if(depth##ax>depth##ox){\
			if(depth##ax>0.f){\
				if(depth##ax*1.05f<outDepth) \
					{\
						dReal sgn=dist##ax<0.f ? -1.f : 1.f;\
						dReal sgn1=sgn*dDOT14(axis,R+ix1)<0.f ? -1.f : 1.f;\
						dReal sgn2=sgn*dDOT14(axis,R+ix2)<0.f ? -1.f : 1.f;\
						for(int ii=0;ii<3;++ii) crpos[ii]=p[ii]+R[ii*4+ix1]*hside[ix1]*sgn1+R[ii*4+ix2]*hside[ix2]*sgn2;\
						if(CrossProjLine14(v##ax,triSideAx##ax,crpos,R+i,hside[i],pos))\
						{\
						outDepth=depth##ax;\
						signum=sgn;\
						outAx[0]=axis[0];\
						outAx[1]=axis[1];\
						outAx[2]=axis[2];\
						code=c+i;\
						}\
					}\
				}\
			else return 0;\
	}\
}

dVector3 crpos;
//#define TEST(ax,ox,c) 
//test_cross_side( dReal* outAx, dReal& outDepth, dReal *pos, dReal& outSignum, u8 &code, u8 c,  const dReal* R, const dReal* hside, const dReal* p,  const dReal* triSideAx, const dReal* vax,   const dReal* vox  )
//if( test_cross_side (outAx,outDepth,pos,signum,code,10,R,hside,p,triSideAx0,v0,v2) )
  // return 0;
TEST(0,2,10)
TEST(1,0,13)
TEST(2,1,16)

#undef TEST

//////////////////////////////////////////////////////////////////////
///if we get to this poit tri touches box
dVector3 norm;
unsigned int ret=1;

if(code==0){
	norm[0]=triAx[0]*signum;
	norm[1]=triAx[1]*signum;
	norm[2]=triAx[2]*signum;

/////////////////////////////////////////// from geom.cpp dCollideBP
  dReal Q1 = -signum*dDOT14(triAx,R+0);
  dReal Q2 = -signum*dDOT14(triAx,R+1);
  dReal Q3 = -signum*dDOT14(triAx,R+2);
  dReal A1 = 2.f*hside[0] * Q1;
  dReal A2 = 2.f*hside[1] * Q2;
  dReal A3 = 2.f*hside[2] * Q3;
  dReal B1 = dFabs(A1);
  dReal B2 = dFabs(A2);
  dReal B3 = dFabs(A3);

  pos[0]=p[0];
  pos[1]=p[1];
  pos[2]=p[2];

#define FOO(i,op) \
  pos[0] op hside[i] * R[0+i]; \
  pos[1] op hside[i] * R[4+i]; \
  pos[2] op hside[i] * R[8+i];
#define BAR(i,iinc) if (A ## iinc > 0) { FOO(i,-=) } else { FOO(i,+=) }
  BAR(0,1);
  BAR(1,2);
  BAR(2,3);
#undef FOO
#undef BAR

///////////////////////////////////////////////////////////


#define TRI_CONTAIN_POINT(pos)	{\
 dVector3 cross0, cross1, cross2;\
 dReal ds0,ds1,ds2;\
 \
  dCROSS(cross0,=,triAx,triSideAx0);\
  ds0=dDOT(cross0,v0);\
\
  dCROSS(cross1,=,triAx,triSideAx1);\
  ds1=dDOT(cross1,v1);\
\
  dCROSS(cross2,=,triAx,triSideAx2);\
  ds2=dDOT(cross2,v2);\
\
  if(dDOT(cross0,pos)-ds0>0.f && \
	 dDOT(cross1,pos)-ds1>0.f && \
	 dDOT(cross2,pos)-ds2>0.f) ++ret;\
}
///////////////////////////////////////////////////////////


  // get the second and third contact points by starting from `p' and going
  // along the two sides with the smallest projected length.

dReal* pdepth;
dContactGeom* prc,*c=CONTACT(contact,ret*skip);
prc=c;
#define FOO(j,op,spoint) \
	c->pos[0] = spoint##[0] op 2.f*hside[j] * R[0+j]; \
	c->pos[1] = spoint##[1] op 2.f*hside[j] * R[4+j]; \
	c->pos[2] = spoint##[2] op 2.f*hside[j] * R[8+j];
#define BAR(side,sideinc,spos,sdepth) \
  {\
  pdepth=&(c->depth);\
  *pdepth =sdepth-B ## sideinc; \
  if (A ## sideinc > 0) { FOO(side,+,spos) } else { FOO(side,-,spos) } \
  prc=c;\
  if (!(*pdepth < 0)) \
	{\
	++ret;\
	c=CONTACT(contact,ret*skip);\
	}\
  }
  //TRI_CONTAIN_POINT(CONTACT(contact,ret*skip)->pos)

   if(B1<B2)
  {
	  BAR(0,1,pos,depth);
	  if(B2<B3) 
	  {
		  BAR(1,2,pos,depth);
		  BAR(0,1,prc->pos,prc->depth);
		  
	  }
	  else
	  {
		  BAR(2,3,pos,depth);
		  BAR(0,1,prc->pos,prc->depth);
	  }
  }
  else
  {
	  BAR(1,2,pos,depth);
	  if(B1<B3)
	  {
		  BAR(0,1,pos,depth);
		  BAR(1,2,prc->pos,prc->depth);
	  }
	  else
	  {		
		  BAR(2,3,pos,depth);
		  BAR(1,2,prc->pos,prc->depth);
		}
  }
  /*

  if (B1 < B2) {
    if (B3 < B1) goto use_side_3; else {
      BAR(0,1,pos);	// use side 1
      if (maxc == 2) goto done;
      if (B2 < B3) goto contact2_2; else goto contact2_3;
    }
  }
  else {
    if (B3 < B2) {
      use_side_3:	// use side 3
      BAR(2,3,pos);
      if (maxc == 2) goto done;
      if (B1 < B2) goto contact2_1; else goto contact2_2;
    }
    else {
      BAR(1,2,pos);	// use side 2
      if (maxc == 2) goto done;
      if (B1 < B3) goto contact2_1; else goto contact2_3;
    }
  }

  contact2_1: BAR(0,1,pos); goto done;
  contact2_2: BAR(1,2,pos); goto done;
  contact2_3: BAR(2,3,pos); goto done;
*/
#undef FOO
#undef FOO1
#undef BAR
#undef TRI_CONTAIN_POINT
 //done: ;

////////////////////////////////////////////////////////////// end (from geom.cpp dCollideBP)

	}
else 
	if(code<=9)
{
	switch((code-1)%3){
	case 0:
	pos[0]=v0[0];
	pos[1]=v0[1];
	pos[2]=v0[2];
	break;
	case 1:
	pos[0]=v1[0];
	pos[1]=v1[1];
	pos[2]=v1[2];
	break;
	case 2:
	pos[0]=v2[0];
	pos[1]=v2[1];
	pos[2]=v2[2];
	break;
	}
switch((code-1)/3){
	case 0:
		{
		norm[0]=R[0]*signum;
		norm[1]=R[4]*signum;
		norm[2]=R[8]*signum;
		}
	break;

	case 1:	
		{
		norm[0]=R[1]*signum;
		norm[1]=R[5]*signum;
		norm[2]=R[9]*signum;
		}
	break;
	case 2:
		{
		norm[0]=R[2]*signum;
		norm[1]=R[6]*signum;
		norm[2]=R[10]*signum;
		}
	break;
	}
}
else {
	norm[0]=outAx[0]*signum;
	norm[1]=outAx[1]*signum;
	norm[2]=outAx[2]*signum;

	//pos[0]=crpos[0];
	//pos[1]=crpos[1];
	//pos[2]=crpos[2];


/////////////
/*
  dReal Q1 = -signum*dDOT14(outAx,R+0);
  dReal Q2 = -signum*dDOT14(outAx,R+1);
  dReal Q3 = -signum*dDOT14(outAx,R+2);
  dReal A1 = 2.f*hside[0] * Q1;
  dReal A2 = 2.f*hside[1] * Q2;
  dReal A3 = 2.f*hside[2] * Q3;
  pos[0]=p[0];
  pos[1]=p[1];
  pos[2]=p[2];

#define FOO(i,op) \
  pos[0] op hside[i] * R[0+i]; \
  pos[1] op hside[i] * R[4+i]; \
  pos[2] op hside[i] * R[8+i];
#define BAR(i,iinc) if (A ## iinc > 0) { FOO(i,-=) } else { FOO(i,+=) }
  BAR(0,1);
  BAR(1,2);
  BAR(2,3);
#undef FOO
#undef BAR
////////////////


switch((code-10)/3){

case 0:
			CrossProjLine1(v0,triSideAx0,pos,R+(code-10),pos);
			if(pos[0]==dInfinity){
									pos[0]=(v1[0]+v0[0])/2.f;
									pos[1]=(v1[1]+v0[1])/2.f;
									pos[2]=(v1[2]+v0[2])/2.f;
								}
break;

case 1:
			CrossProjLine1(v1,triSideAx1,pos,R+(code-13),pos);
			if(pos[0]==dInfinity){
									pos[0]=(v2[0]+v1[0])/2.f;
									pos[1]=(v2[1]+v1[1])/2.f;
									pos[2]=(v2[2]+v1[2])/2.f;
									}
break;

case 2:
			CrossProjLine1(v0,triSideAx2,pos,R+(code-16),pos);
			if(pos[0]==dInfinity){
									pos[0]=(v2[0]+v0[0])/2.f;
									pos[1]=(v2[1]+v0[1])/2.f;
									pos[2]=(v2[2]+v0[2])/2.f;
								}

}
*/
}

if(dDOT(norm,triAx)>0.f)	return 0;


//if(0!=code){
contact->pos[0] = pos[0];
contact->pos[1] = pos[1];
contact->pos[2] = pos[2];


//contact->pos[0] = crossprg[0];
//contact->pos[1] = crossprg[1];
//contact->pos[2] = crossprg[2];

contact->depth = outDepth;

//}
 for (u32 i=0; i<ret; ++i) {
    CONTACT(contact,i*skip)->g1 = const_cast<dxGeom*> (o2);
    CONTACT(contact,i*skip)->g2 = const_cast<dxGeom*> (o1);
	CONTACT(contact,i*skip)->normal[0] = norm[0];
	CONTACT(contact,i*skip)->normal[1] = norm[1];
	CONTACT(contact,i*skip)->normal[2] = norm[2];
	SURFACE(contact,i*skip)->mode=T->T->material;
 }
 if(ret&&dGeomGetUserData(o1)->callback)dGeomGetUserData(o1)->callback(T->T,contact);
 return ret;

}

