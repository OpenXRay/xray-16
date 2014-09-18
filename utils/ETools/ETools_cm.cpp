#include "stdafx.h"

#include "ETools.h"

class CCubeMapHelper
{
public:
	enum ECubeSide{
		CUBE_POSITIVE_X	= 0,
		CUBE_NEGATIVE_X	= 1,
		CUBE_POSITIVE_Y	= 2,
		CUBE_NEGATIVE_Y	= 3,
		CUBE_POSITIVE_Z	= 4,
		CUBE_NEGATIVE_Z	= 5,
		CUBE_SIDE_COUNT	= 6,
		CUBE_forced_u32	= u32(-1)
	};
protected:
	Fplane		planes[CUBE_SIDE_COUNT];
public:
	CCubeMapHelper		()
	{
		planes[CUBE_POSITIVE_X].build	(Fvector().set(+0.5f,0,0),Fvector().set(-1,0,0));
		planes[CUBE_NEGATIVE_X].build   (Fvector().set(-0.5f,0,0),Fvector().set(+1,0,0));
		planes[CUBE_POSITIVE_Y].build   (Fvector().set(0,+0.5f,0),Fvector().set(0,-1,0));
		planes[CUBE_NEGATIVE_Y].build   (Fvector().set(0,-0.5f,0),Fvector().set(0,+1,0));
		planes[CUBE_POSITIVE_Z].build   (Fvector().set(0,0,+0.5f),Fvector().set(0,0,-1));
		planes[CUBE_NEGATIVE_Z].build   (Fvector().set(0,0,-0.5f),Fvector().set(0,0,+1));
	}
	void 		vector_from_point	(Fvector3& normal, ECubeSide side, u32 x, u32 y, u32 width, u32 height)
	{
		float w,h;
		w = (float)x / ((float)(width - 1));
		w *= 2.0f;    w -= 1.0f;
		h = (float)y / ((float)(height - 1));
		h *= 2.0f;    h -= 1.0f;
		switch(side)
		{
		case CUBE_POSITIVE_X: normal.set(	+1.0f,	-h,     -w      );	break;
		case CUBE_NEGATIVE_X: normal.set(   -1.0f,	-h,     +w		);	break;
		case CUBE_POSITIVE_Y: normal.set(   +w,   	+1.0f, 	+h		);	break;
		case CUBE_NEGATIVE_Y: normal.set(   +w,		-1.0f, 	-h		);	break;
		case CUBE_POSITIVE_Z: normal.set(   +w,		-h,     +1.0f	);  break;
		case CUBE_NEGATIVE_Z: normal.set(   -w,		-h, 	-1.0f	);	break;
		default:                                                        break;
		}
		// Normalize and store
		normal.normalize	();
	}
	u32& 		pixel_from_side		(u32* pixels, u32 width, u32 height, u32 side, u32 x, u32 y)
	{
		u32	offset			= width*side;
		offset 				+= (y*width*6+x); VERIFY(offset<width*6*height);
		return pixels[offset];
	}
	void		xys_from_vector		(u32& ux, u32& uy, u32& t_side, const Fvector& n, u32* pixels, u32 width, u32 height)
	{
		float t_dist		= flt_max;
		t_side				= CUBE_POSITIVE_X;
		for (u32 side=CUBE_POSITIVE_X; side<=CUBE_NEGATIVE_Z; side++){
			float cur_dist				= flt_max;
			if (planes[side].intersectRayDist(Fvector().set(0,0,0),n,cur_dist)){
				if (cur_dist<t_dist){	t_dist=cur_dist; t_side=side; }
			}
		}    
		Fvector coord;		
		coord.mad			(Fvector().set(0,0,0),n,t_dist);

		float x,y;
		switch(t_side){
		case CUBE_POSITIVE_X: 
			x				= -coord.z;
			y				= -coord.y; 
			break;
		case CUBE_NEGATIVE_X: 
			x				= coord.z;
			y				= -coord.y;
			break;
		case CUBE_POSITIVE_Y: 
			x				= coord.x;
			y				= coord.z;
			break;
		case CUBE_NEGATIVE_Y: 
			x				= coord.x;
			y				= coord.z;
			break;
		case CUBE_POSITIVE_Z: 
			x				= coord.x;
			y				= -coord.y;
			break;
		case CUBE_NEGATIVE_Z: 
			x				= -coord.x;
			y				= -coord.y;
			break;
		}
		clamp				(x,-0.5f,0.5f);
		clamp				(y,-0.5f,0.5f);
		ux	 				= iFloor((x+0.5f)*(width-1)+0.5f);	
		uy 					= iFloor((y+0.5f)*(height-1)+0.5f);	
	}
	u32& 		pixel_from_vector	(const Fvector& n, u32* pixels, u32 width, u32 height)
	{
		u32					ux,uy,side;
		xys_from_vector		(ux,uy,side,n,pixels,width,height);
		return pixel_from_side(pixels,width,height,side,ux,uy);
	}
	void	scale_map		(u32* src_data, u32 src_width, u32 src_height, u32* dst_data, u32 dst_width, u32 dst_height, float sample_factor, ETOOLS::pb_callback cb, void* pb_data)
	{
		VERIFY				((src_width==src_height)&&(dst_width==dst_height));
		Fvector3 normal;
		Fvector3 dir;
		float d_size		= sample_factor*float(src_width)/dst_width;
		float t_angle		= sample_factor*PI_DIV_2/float(dst_width);   VERIFY(t_angle<PI_DIV_2);
		float d_angle		= t_angle/d_size;
		float h_angle		= t_angle/2;
		float pb_weight		= 1.f/(CUBE_SIDE_COUNT*dst_height*dst_width);
		float pb_cur		= 0.f;
		for (u32 side=CUBE_POSITIVE_X; side<CUBE_SIDE_COUNT; side++){
			for (u32 y_dst=0; y_dst<dst_height; y_dst++){
				for (u32 x_dst=0; x_dst<dst_width; x_dst++){
					pb_cur	+= pb_weight;
					cb		(pb_data,pb_cur);
					vector_from_point		(normal,ECubeSide(side),x_dst,y_dst,dst_width,dst_height);
					u32& out				= pixel_from_side(dst_data,dst_width,dst_height,side,x_dst,y_dst);
					Fcolor sum, sample_color;
					sum.set					(0,0,0,0);
					float 	src_h,src_p;
					normal.getHP			(src_h,src_p);
					u32 ds					= 0;
					for (float p=src_p-h_angle; p<src_p+h_angle; p+=d_angle){
						for (float h=src_h-h_angle; h<src_h+h_angle; h+=d_angle){
							dir.setHP		(h,p);
							sample_color.set(pixel_from_vector(dir,src_data,src_width,src_height));
							sum.r			+= sample_color.r*sample_color.r;
							sum.g			+= sample_color.g*sample_color.g;
							sum.b			+= sample_color.b*sample_color.b;
							sum.a			+= sample_color.a*sample_color.a;
							ds++;
						}
					}
					sum.mul_rgba			(1.f/ds);
					sum.r					= _sqrt(sum.r);
					sum.g					= _sqrt(sum.g);
					sum.b					= _sqrt(sum.b);
					sum.a					= _sqrt(sum.a);
					out						= sum.get();
				}
			}
		}
	}
};           

CCubeMapHelper cm;
extern "C"{
	namespace ETOOLS{
		ETOOLS_API void  __stdcall SimplifyCubeMap	(u32* src_data, u32 src_width, u32 src_height, u32* dst_data, u32 dst_width, u32 dst_height, float sample_factor, pb_callback cb, void* pb_data)
		{
			cm.scale_map(src_data, src_width, src_height, dst_data, dst_width, dst_height, sample_factor, cb, pb_data);
		}
	}
}
