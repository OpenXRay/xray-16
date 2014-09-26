#include "stdafx.h"
#include "build.h"
#include "tga.h"

#include "../xrLc_light/xrThread.h"
#include "../xrLc_light/hash2D.h"
#include "../xrLc_light/lm_layer.h"
#include "../xrLC_Light/light_point.h"
#include "../xrLc_light/xrdeflector.h"
#include "../xrLC_Light/xrLC_GlobalData.h"
#include "../xrLC_Light/xrface.h"

#include "../../xrcdb/xrcdb.h"

class ImplicitDeflector
{
public:
	b_BuildTexture*			texture;
	lm_layer				lmap;
	vecFace					faces;
	
	ImplicitDeflector() : texture(0)
	{
	}
	~ImplicitDeflector()
	{
		Deallocate	();
	}
	
	void			Allocate	()
	{
		lmap.create	(Width(),Height());
	}
	void			Deallocate	()
	{
		lmap.destroy();
	}
	
	u32			Width	()						{ return texture->dwWidth; }
	u32			Height	()						{ return texture->dwHeight; }
	
	u32&		Texel	(u32 x, u32 y)			{ return texture->pSurface[y*Width()+x]; }
	base_color& Lumel	(u32 x, u32 y)			{ return lmap.surface[y*Width()+x];	}
	u8&			Marker	(u32 x, u32 y)			{ return lmap.marker [y*Width()+x];	}
	
	void	Bounds	(u32 ID, Fbox2& dest)
	{
		Face* F		= faces[ID];
		_TCF& TC	= F->tc[0];
		dest.min.set	(TC.uv[0]);
		dest.max.set	(TC.uv[0]);
		dest.modify		(TC.uv[1]);
		dest.modify		(TC.uv[2]);
	}
	void	Bounds_Summary (Fbox2& bounds)
	{
		bounds.invalidate();
		for (u32 I=0; I<faces.size(); I++)
		{
			Fbox2	B;
			Bounds	(I,B);
			bounds.merge(B);
		}
	}
};


DEF_MAP(Implicit,u32,ImplicitDeflector);

typedef hash2D <Face*,384,384>		IHASH;
static IHASH*						ImplicitHash;

class ImplicitThread : public CThread
{
public:
	ImplicitDeflector*	DATA;			// Data for this thread
	u32					y_start,y_end;

	ImplicitThread		(u32 ID, ImplicitDeflector* _DATA, u32 _y_start, u32 _y_end) : CThread (ID)
	{
		DATA			= _DATA;
		y_start			= _y_start;
		y_end			= _y_end;
	}
	virtual void		Execute	()
	{
		// Priority
		SetThreadPriority		(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		Sleep					(0);
		R_ASSERT				(DATA);
		ImplicitDeflector&		defl	= *DATA;
		CDB::COLLIDER			DB;
		
		// Setup variables
		Fvector2	dim,half;
		dim.set		(float(defl.Width()),float(defl.Height()));
		half.set	(.5f/dim.x,.5f/dim.y);
		
		// Jitter data
		Fvector2	JS;
		JS.set		(.499f/dim.x, .499f/dim.y);
		u32			Jcount;
		Fvector2*	Jitter;
		Jitter_Select(Jitter, Jcount);
		
		// Lighting itself
		DB.ray_options	(0);
		for (u32 V=y_start; V<y_end; V++)
		{
			for (u32 U=0; U<defl.Width(); U++)
			{
				base_color_c	C;
				u32				Fcount	= 0;
				
				try {
					for (u32 J=0; J<Jcount; J++) 
					{
						// LUMEL space
						Fvector2				P;
						P.x						= float(U)/dim.x + half.x + Jitter[J].x * JS.x;
						P.y						= float(V)/dim.y + half.y + Jitter[J].y * JS.y;
						xr_vector<Face*>& space	= ImplicitHash->query(P.x,P.y);
						
						// World space
						Fvector wP,wN,B;
						for (vecFaceIt it=space.begin(); it!=space.end(); it++)
						{
							Face	*F	= *it;
							_TCF&	tc	= F->tc[0];
							if (tc.isInside(P,B)) 
							{
								// We found triangle and have barycentric coords
								Vertex	*V1 = F->v[0];
								Vertex	*V2 = F->v[1];
								Vertex	*V3 = F->v[2];
								wP.from_bary(V1->P,V2->P,V3->P,B);
								wN.from_bary(V1->N,V2->N,V3->N,B);
								wN.normalize();
								LightPoint	(&DB, lc_global_data()->RCAST_Model(), C, wP, wN, pBuild->L_static(), (lc_global_data()->b_nosun()?LP_dont_sun:0), F);
								Fcount		++;
							}
						}
					} 
				} catch (...)
				{
					clMsg("* THREAD #%d: Access violation. Possibly recovered.",thID);
				}
				if (Fcount) {
					// Calculate lighting amount
					C.scale				(Fcount);
					C.mul				(.5f);
					defl.Lumel(U,V)._set(C);
					defl.Marker(U,V)	= 255;
				} else {
					defl.Marker(U,V)	= 0;
				}
			}
			thProgress	= float(V - y_start) / float(y_end-y_start);
		}
	}
};

//#pragma optimize( "g", off )

#define	NUM_THREADS	8
void CBuild::ImplicitLighting()
{
	if (g_params().m_quality==ebqDraft) return;

	Implicit		calculator;
	ImplicitHash	= xr_new<IHASH>	();
	
	// Sorting
	Status("Sorting faces...");
	for (vecFaceIt I=lc_global_data()->g_faces().begin(); I!=lc_global_data()->g_faces().end(); I++)
	{
		Face* F = *I;
		if (F->pDeflector)				continue;
		if (!F->hasImplicitLighting())	continue;
		
		Progress		(float(I-lc_global_data()->g_faces().begin())/float(lc_global_data()->g_faces().size()));
		b_material&		M	= materials()		[F->dwMaterial];
		u32				Tid = M.surfidx;
		b_BuildTexture*	T	= &(textures()[Tid]);
		
		Implicit_it		it	= calculator.find(Tid);
		if (it==calculator.end()) 
		{
			ImplicitDeflector	ImpD;
			ImpD.texture		= T;
			ImpD.faces.push_back(F);
			calculator.insert	(mk_pair(Tid,ImpD));
		} else {
			ImplicitDeflector&	ImpD = it->second;
			ImpD.faces.push_back(F);
		}
	}
	
	// Lighing
	for (Implicit_it imp=calculator.begin(); imp!=calculator.end(); imp++)
	{
		ImplicitDeflector& defl = imp->second;
		Status			("Lighting implicit map '%s'...",defl.texture->name);
		Progress		(0);
		defl.Allocate	();
		
		// Setup cache
		Progress					(0);
		Fbox2 bounds;
		defl.Bounds_Summary			(bounds);
		ImplicitHash->initialize	(bounds,defl.faces.size());
		for (u32 fid=0; fid<defl.faces.size(); fid++)
		{
			Face* F				= defl.faces[fid];
			F->AddChannel		(F->tc[0].uv[0],F->tc[0].uv[1],F->tc[0].uv[2]); // make compatible format with LMAPs
			defl.Bounds			(fid,bounds);
			ImplicitHash->add	(bounds,F);
		}

		// Start threads
		CThreadManager			tmanager;
		u32	stride				= defl.Height()/NUM_THREADS;
		for (u32 thID=0; thID<NUM_THREADS; thID++)
			tmanager.start		(xr_new<ImplicitThread> (thID,&defl,thID*stride,thID*stride+stride));
		tmanager.wait			();

		// Expand
		Status	("Processing lightmap...");
		for (u32 ref=254; ref>0; ref--)	if (!ApplyBorders(defl.lmap,ref)) break;

		Status	("Mixing lighting with texture...");
		{
			b_BuildTexture& TEX		=	*defl.texture;
			VERIFY					(TEX.pSurface);
			u32*			color	= TEX.pSurface;
			for (u32 V=0; V<defl.Height(); V++)	{
				for (u32 U=0; U<defl.Width(); U++)	{
					// Retreive Texel
					float	h	= defl.Lumel(U,V).h._r();
					u32 &C		= color[V*defl.Width() + U];
					C			= subst_alpha(C,u8_clr(h));
				}
			}
		}

		// base
		Status	("Saving base...");
		{
			string_path				name, out_name;
			sscanf					(strstr(Core.Params,"-f")+2,"%s",name);
			R_ASSERT				(name[0] && defl.texture);
			b_BuildTexture& TEX		=	*defl.texture;
			strconcat				(sizeof(out_name),out_name,name,"\\",TEX.name,".dds");
			FS.update_path			(out_name,_game_levels_,out_name);
			clMsg					("Saving texture '%s'...",out_name);
			VerifyPath				(out_name);
			BYTE* raw_data			=	LPBYTE(TEX.pSurface);
			u32	w					=	TEX.dwWidth;
			u32	h					=	TEX.dwHeight;
			u32	pitch				=	w*4;
			STextureParams			fmt	= TEX.THM;
			fmt.fmt					= STextureParams::tfDXT5;
			fmt.flags.set			(STextureParams::flDitherColor,		FALSE);
			fmt.flags.set			(STextureParams::flGenerateMipMaps,	FALSE);
			fmt.flags.set			(STextureParams::flBinaryAlpha,		FALSE);
			DXTCompress				(out_name,raw_data,0,w,h,pitch,&fmt,4);
		}

		// lmap
		Status	("Saving lmap...");
		{
			xr_vector<u32>			packed;
			defl.lmap.Pack			(packed);

			string_path				name, out_name;
			sscanf					(strstr(GetCommandLine(),"-f")+2,"%s",name);
			b_BuildTexture& TEX		=	*defl.texture;
			strconcat				(sizeof(out_name),out_name,name,"\\",TEX.name,"_lm.dds");
			FS.update_path			(out_name,_game_levels_,out_name);
			clMsg					("Saving texture '%s'...",out_name);
			VerifyPath				(out_name);
			BYTE* raw_data			= LPBYTE(&*packed.begin());
			u32	w					= TEX.dwWidth;
			u32	h					= TEX.dwHeight;
			u32	pitch				= w*4;
			STextureParams			fmt;
			fmt.fmt					= STextureParams::tfDXT5;
			fmt.flags.set			(STextureParams::flDitherColor,		FALSE);
			fmt.flags.set			(STextureParams::flGenerateMipMaps,	FALSE);
			fmt.flags.set			(STextureParams::flBinaryAlpha,		FALSE);
			DXTCompress				(out_name,raw_data,0,w,h,pitch,&fmt,4);
		}
		defl.Deallocate				();
	}

	xr_delete			(ImplicitHash);
	calculator.clear	();
}
