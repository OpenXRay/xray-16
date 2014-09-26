#include "stdafx.h"

#include "etools.h"

#include "../../include/xrrender/kinematics.h"
#define ECORE_API 
#include "../../xrEngine/bone.h"

namespace ETOOLS{


	enum box_axis
	{
		a_px, a_nx,
		a_py, a_ny,
		a_pz, a_nz,
		a_none
	} ax;

 IC  static bool		clip		(float fDenom, float fNumer, float& rfT0, float& rfT1, bool &code0, bool &code1 )
    {
        // Return value is 'true' if line segment intersects the current test
        // plane.  Otherwise 'false' is returned in which case the line segment
        // is entirely clipped.
		code0 = code1 = false;
        if ( fDenom > 0.0f ){
            if ( fNumer > fDenom*rfT1 ) return false;
            if ( fNumer > fDenom*rfT0 )
			{
				rfT0 = fNumer/fDenom;
				code0 = true;
			}
            return true;
        }else if ( fDenom < 0.0f ){
            if ( fNumer > fDenom*rfT0 ) return false;
            if ( fNumer > fDenom*rfT1 )
			{
				rfT1 = fNumer/fDenom;
				code1 = true;
			}
            return true;
        }else{
            return fNumer <= 0.0f;
        }
    }
	IC  static bool		clip		(float fDenom, float fNumer, float& rfT0, float& rfT1, box_axis &ax0, box_axis &ax1, box_axis ax )
	{
		bool b0 =false, b1 = false;
		if( !clip( fDenom, fNumer, rfT0, rfT1, b0, b1 ) )
			return false;

		if( b0 )
			ax0 = ax;

		if( b1 )
			ax1 = ax;

		return true;
	}
	static bool 	intersect	(const Fvector& start, const Fvector& dir, const Fvector& extent, float& rfT0, float& rfT1, box_axis &ax0, box_axis &ax1 )
	{
		//float fSaveT0 = rfT0, fSaveT1 = rfT1;
		

		
		
		bool bNotEntirelyClipped =
				clip(+dir.x,-start.x-extent[0],rfT0,rfT1, ax0, ax1, a_nx )&&
				clip(-dir.x,+start.x-extent[0],rfT0,rfT1, ax0, ax1, a_px )&&
				clip(+dir.y,-start.y-extent[1],rfT0,rfT1, ax0, ax1, a_ny )&&
				clip(-dir.y,+start.y-extent[1],rfT0,rfT1, ax0, ax1, a_py )&&
				clip(+dir.z,-start.z-extent[2],rfT0,rfT1, ax0, ax1, a_nz )&&
				clip(-dir.z,+start.z-extent[2],rfT0,rfT1, ax0, ax1, a_pz );
			
		return bNotEntirelyClipped && ( ax0 != a_none || ax1 != a_none ); //( rfT0 != fSaveT0 || rfT1 != fSaveT1 );
	}




 
bool intersect( const Fobb& box ,const Fvector& origin, const Fvector &direction, float &dist, Fvector &norm  )
{
	
	        // convert ray to box coordinates
        Fvector kDiff; 
        kDiff.sub(origin,box.m_translate);
        Fvector kOrigin;
        kOrigin.set(kDiff.dotproduct(box.m_rotate.i), kDiff.dotproduct(box.m_rotate.j), kDiff.dotproduct(box.m_rotate.k));
        Fvector kDirection;
        kDirection.set(direction.dotproduct(box.m_rotate.i),direction.dotproduct(box.m_rotate.j),direction.dotproduct(box.m_rotate.k));

        float fT0 = 0.0f, fT1 = type_max(float);
		box_axis ax0 = a_none;
		box_axis ax1 = a_none;
		box_axis ax = a_none;
        if ( intersect( kOrigin, kDirection, box.m_halfsize, fT0, fT1, ax0, ax1 ) )
		{
            bool bPick=false;

            if ( fT0 > 0.0f ){

                if ( fT0<dist )
					{ dist = fT0; ax = ax0; bPick=true;}

                if ( fT1<dist )
					{ dist = fT1; ax = ax1; bPick=true;}

            }else{

                if (fT1<dist)
					{ dist = fT1; ax = ax1; bPick=true;}

            }

			if( bPick )
			{
				switch( ax )
				{
				case	a_px: norm.set(box.m_rotate.i); break; case	 a_nx: norm.invert( box.m_rotate.i ); break;
				case	a_py: norm.set(box.m_rotate.j); break; case	 a_ny: norm.invert( box.m_rotate.j ); break;
				case	a_pz: norm.set(box.m_rotate.k); break; case	 a_nz: norm.invert( box.m_rotate.k ); break;
				case	a_none: NODEFAULT;
				}
			}

            return bPick;
        }

        return false;
}

bool intersect( const Fsphere& sphere ,const Fvector& origin, const Fvector &direction, float &dist, Fvector &norm  )
{
	bool b_result = !!sphere.intersect2( origin, direction, dist );
	if( b_result )
	{
		const Fvector pt = Fvector().mad( origin, direction, dist );
		norm.sub( pt, sphere.P ).normalize_safe();
	}
	
	return b_result;
}
bool intersect( const Fcylinder& cylinder ,const Fvector& origin, const Fvector &direction, float &dist, Fvector &norm  )
{
	
	float					afT[2] ;
	Fcylinder::ecode 		code[2] =	{ Fcylinder::cyl_none, Fcylinder::cyl_none };
	Fcylinder::ecode		r_code =	Fcylinder::cyl_none;
   	bool		b_result	= false;
	int cnt;
	if ( 0!=( cnt=cylinder.intersect(origin,direction,afT,code) ) )
	{
		//bool		o_inside	= false;

		for (int k=0; k<cnt; k++)
		{
			if (afT[k]<0.f)		
					continue;	
			
			if (afT[k]<dist)	
			{
				dist=afT[k];		
				r_code = code[k];
				b_result=true;				
			}
		}
		
	}else{
		return		false;
	}
	
	if( b_result )
	{
		const Fvector pt	= Fvector().mad( origin, direction, dist );
		const Fvector c_pt	= Fvector().sub( pt, cylinder.m_center );
		VERIFY( r_code != Fcylinder::cyl_none );

		if( r_code == Fcylinder::cyl_cap )
		{
			if( c_pt.dotproduct( cylinder.m_direction ) > 0.f )
				norm.set( cylinder.m_direction );
			else
				norm.invert( cylinder.m_direction );

		}
		if( r_code == Fcylinder::cyl_wall )
		{
			const Fvector r_dir = Fvector().mad( pt, cylinder.m_direction, -pt.dotproduct(cylinder.m_direction) );
			norm = Fvector().normalize_safe( r_dir );
		}

	}
	
	return b_result;
}

 bool bone_intersect			( u16 bone, const IKinematics& K, const Fvector& origin, const Fvector &direction, float &dist, Fvector &norm  )
 {
	const IBoneData &d = K.GetBoneData( bone );
	const Fmatrix	&bone_transform = K.LL_GetTransform(bone);
	const Fmatrix	bone_invert_transform = Fmatrix().invert(bone_transform);

	Fvector l_origin;
	bone_invert_transform.transform_tiny( l_origin, origin );
	Fvector l_direction;
	bone_invert_transform.transform_dir( l_direction, direction );
	const SBoneShape &bone_shape = d.get_shape();
	
	bool result = false;
	switch ( bone_shape.type )
	{
	case	SBoneShape::stNone		: return false;
	case	SBoneShape::stBox		: result = intersect( bone_shape.box,		l_origin, l_direction, dist, norm ); break;
	case	SBoneShape::stSphere	: result = intersect( bone_shape.sphere,	l_origin, l_direction, dist, norm ); break;
	case	SBoneShape::stCylinder	: result = intersect( bone_shape.cylinder,	l_origin, l_direction, dist, norm ); break;
	default							: NODEFAULT ; return false; 
    };

	if( result )
		bone_transform.transform_dir( norm );
	
	return result;
 }
	

 
 bool	__stdcall	intersect			( const IKinematics& K, const Fvector& origin, const Fvector &direction, u16 &bone_id,  float &dist, Fvector &norm  )
 {
	
	 bool b_res = false;
	 const u16 bc = K.LL_BoneCount();
	 for( u16 i = 0; i< bc; ++i )
	 {
		
		 float l_dist = FLT_MAX;
		 Fvector l_norm;
		
		 if( !bone_intersect( i, K, origin, direction, l_dist, l_norm ) )
			 continue;

		
		 if( !b_res || l_dist < dist )
		 {
			dist = l_dist;
			norm = l_norm;
			bone_id = i;
		 }
		 b_res = true;
	 }
	 return b_res;
 }

bool	__stdcall	intersect			( const Fmatrix &object_transform, const IKinematics& K, const Fvector& origin, const Fvector &direction, u16 &bone_id,  float &dist, Fvector &norm  )
{
		
		const Fmatrix inverce_object_transform = Fmatrix().invert( object_transform );

		Fvector l_origin ;
		inverce_object_transform.transform_tiny( l_origin, origin );
		Fvector l_direction;
		inverce_object_transform.transform_dir( l_direction, direction );

		bool res = intersect( K, l_origin, l_direction, bone_id, dist, norm );

		if( res )
		{
			
			object_transform.transform_dir( norm );
		}

		return res;
}

}

