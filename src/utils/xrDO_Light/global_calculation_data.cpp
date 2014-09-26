#include "stdafx.h"

#include "global_calculation_data.h"

#include "../shader_xrlc.h"

global_claculation_data	gl_data;




template <class T>
void transfer(const char *name, xr_vector<T> &dest, IReader& F, u32 chunk)
{
	IReader*	O	= F.open_chunk(chunk);
	u32		count	= O?(O->length()/sizeof(T)):0;
	clMsg			("* %16s: %d",name,count);
	if (count)  
	{
		dest.reserve(count);
		dest.insert	(dest.begin(), (T*)O->pointer(), (T*)O->pointer() + count);
	}
	if (O)		O->close	();
}

extern u32*		Surface_Load	(char* name, u32& w, u32& h);
extern void		Surface_Init	();

// 



void global_claculation_data::xrLoad()
{
	string_path					N;
	FS.update_path				( N, "$game_data$", "shaders_xrlc.xr" );
	g_shaders_xrlc				= xr_new<Shader_xrLC_LIB> ();
	g_shaders_xrlc->Load		( N );

	// Load CFORM
	{
		FS.update_path			(N,"$level$","build.cform");
		IReader*			fs = FS.r_open("$level$","build.cform");
		
		R_ASSERT			(fs->find_chunk(0));
		hdrCFORM			H;
		fs->r				(&H,sizeof(hdrCFORM));
		R_ASSERT			(CFORM_CURRENT_VERSION==H.version);
		
		Fvector*	verts	= (Fvector*)fs->pointer();
		CDB::TRI*	tris	= (CDB::TRI*)(verts+H.vertcount);
		RCAST_Model.build	( verts, H.vertcount, tris, H.facecount );
		Msg("* Level CFORM: %dK",RCAST_Model.memory()/1024);

		g_rc_faces.resize	(H.facecount);
		R_ASSERT(fs->find_chunk(1));
		fs->r				(&*g_rc_faces.begin(),g_rc_faces.size()*sizeof(b_rc_face));

		LevelBB.set			(H.aabb);
	}
	
	{
		slots_data.Load( );
	}
	// Lights
	{
		IReader*			fs = FS.r_open("$level$","build.lights");
		IReader*			F;	u32 cnt; R_Light* L;

		// rgb
		F		=			fs->open_chunk		(0);
		cnt		=			F->length()/sizeof(R_Light);
		L		=			(R_Light*)F->pointer();
		g_lights.rgb.assign	(L,L+cnt);
		F->close			();

		// hemi
		F		=			fs->open_chunk		(1);
		cnt		=			F->length()/sizeof(R_Light);
		L		=			(R_Light*)F->pointer();
		g_lights.hemi.assign(L,L+cnt);
		F->close			();

		// sun
		F		=			fs->open_chunk		(2);
		cnt		=			F->length()/sizeof(R_Light);
		L		=			(R_Light*)F->pointer();
		g_lights.sun.assign	(L,L+cnt);
		F->close			();

		FS.r_close			(fs);
	}

	
	// Load level data
	{
		IReader*	fs		= FS.r_open ("$level$","build.prj");
		IReader*	F;

		// Version
		u32 version;
		fs->r_chunk			(EB_Version,&version);
		R_ASSERT(XRCL_CURRENT_VERSION==version);

		// Header
		fs->r_chunk			(EB_Parameters,&g_params);

		// Load level data
		transfer("materials",	g_materials,			*fs,		EB_Materials);
		transfer("shaders_xrlc",g_shader_compile,		*fs,		EB_Shaders_Compile);
		post_process_materials( *g_shaders_xrlc, g_shader_compile, g_materials );
		// process textures
#ifdef	DEBUG
		xr_vector<b_texture> dbg_textures;
#endif
		Status			("Processing textures...");
		{
			Surface_Init		();
			F = fs->open_chunk	(EB_Textures);
			u32 tex_count	= F->length()/sizeof(b_texture);
			for (u32 t=0; t<tex_count; t++)
			{
				Progress		(float(t)/float(tex_count));

				b_texture		TEX;
				F->r			(&TEX,sizeof(TEX));
#ifdef	DEBUG
				dbg_textures.push_back( TEX );
#endif

				b_BuildTexture	BT;
				CopyMemory		(&BT,&TEX,sizeof(TEX));

				// load thumbnail
				LPSTR N			= BT.name;
				if (strchr(N,'.')) *(strchr(N,'.')) = 0;
				strlwr			(N);

				if (0==xr_strcmp(N,"level_lods"))	{
					// HACK for merged lod textures
					BT.dwWidth	= 1024;
					BT.dwHeight	= 1024;
					BT.bHasAlpha= TRUE;
					BT.pSurface	= 0;
				} else {
					strcat			(N,".thm");
					IReader* THM	= FS.r_open("$game_textures$",N);
					R_ASSERT2		(THM,	N);

					// version
					u32 version				= 0;
					R_ASSERT				(THM->r_chunk(THM_CHUNK_VERSION,&version));
					// if( version!=THM_CURRENT_VERSION )	FATAL	("Unsupported version of THM file.");

					// analyze thumbnail information
					R_ASSERT(THM->find_chunk(THM_CHUNK_TEXTUREPARAM));
					THM->r                  (&BT.THM.fmt,sizeof(STextureParams::ETFormat));
					BT.THM.flags.assign		(THM->r_u32());
					BT.THM.border_color		= THM->r_u32();
					BT.THM.fade_color		= THM->r_u32();
					BT.THM.fade_amount		= THM->r_u32();
					BT.THM.mip_filter		= THM->r_u32();
					BT.THM.width			= THM->r_u32();
					BT.THM.height           = THM->r_u32();
					BOOL			bLOD=FALSE;
					if (N[0]=='l' && N[1]=='o' && N[2]=='d' && N[3]=='\\') bLOD = TRUE;

					// load surface if it has an alpha channel or has "implicit lighting" flag
					BT.dwWidth				= BT.THM.width;
					BT.dwHeight				= BT.THM.height;
					BT.bHasAlpha			= BT.THM.HasAlphaChannel();
					BT.pSurface				= 0;
					if (!bLOD) 
					{
						if (BT.bHasAlpha || BT.THM.flags.test(STextureParams::flImplicitLighted))
						{
							clMsg		("- loading: %s",N);
							u32			w=0, h=0;
							BT.pSurface = Surface_Load(N,w,h); 
							R_ASSERT2	(BT.pSurface,"Can't load surface");
							if ((w != BT.dwWidth) || (h != BT.dwHeight))
								Msg		("! THM doesn't correspond to the texture: %dx%d -> %dx%d", BT.dwWidth, BT.dwHeight, w, h);
							BT.Vflip	();
						} else {
							// Free surface memory
						}
					}
				}

				// save all the stuff we've created
				g_textures.push_back	(BT);
			}
		}
	}
}
