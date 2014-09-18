#ifndef SHADER_XRLC_H
#define SHADER_XRLC_H
#pragma once

struct Shader_xrLC
{
public:
	enum {
		flCollision			= 1<<0,
		flRendering			= 1<<1,
		flOptimizeUV		= 1<<2,
		flLIGHT_Vertex		= 1<<3,
		flLIGHT_CastShadow	= 1<<4,
		flLIGHT_Sharp		= 1<<5,
	};
	struct Flags {
		u32 bCollision				: 1;
		u32 bRendering				: 1;
		u32 bOptimizeUV				: 1;
		u32 bLIGHT_Vertex			: 1;
		u32 bLIGHT_CastShadow		: 1;
		u32 bLIGHT_Sharp			: 1;
	};
public:
	char		Name		[128];
	union{
		Flags32	m_Flags;
        Flags	flags;
    };
	float		vert_translucency;
	float		vert_ambient;
	float		lm_density;

	Shader_xrLC()	{
		strcpy					(Name,"unknown");
		m_Flags.assign			(0);
		flags.bCollision		= TRUE;
		flags.bRendering		= TRUE;
		flags.bOptimizeUV		= TRUE;
		flags.bLIGHT_Vertex		= FALSE;
		flags.bLIGHT_CastShadow = TRUE;
		flags.bLIGHT_Sharp		= TRUE;
		vert_translucency		= .5f;
		vert_ambient			= .0f;
		lm_density				= 1.f;
	}
};

DEFINE_VECTOR(Shader_xrLC,Shader_xrLCVec,Shader_xrLCIt);
class Shader_xrLC_LIB
{
	Shader_xrLCVec			library;
public:
	void					Load	(LPCSTR name)
	{
		IReader* fs			= FS.r_open(name);
		if(NULL==fs){
			string256 inf;
			extern  HWND logWindow;
			sprintf				(inf,"Build failed!\nCan't load shaders library: '%s'",name);
//			clMsg				(inf);
//			MessageBox			(logWindow,inf,"Error!",MB_OK|MB_ICONERROR);
			FATAL				(inf);
			return;
		};

		int count			= fs->length()/sizeof(Shader_xrLC);
		R_ASSERT			(int(fs->length()) == int(count*sizeof(Shader_xrLC)));
		library.resize		(count);
		fs->r				(&*library.begin(),fs->length());
        FS.r_close			(fs);
	}
	bool					Save	(LPCSTR name)
	{
		IWriter* F			= FS.w_open(name);
        if (F){
			F->w			(&*library.begin(),(u32)library.size()*sizeof(Shader_xrLC));
    	    FS.w_close		(F);
            return 			true;
        }else{
        	return 			false;
        }
	}
	void					Unload	()
	{
		library.clear		();
	}
	u32						GetID	(LPCSTR name)
	{
		for (Shader_xrLCIt it=library.begin(); it!=library.end(); it++)
			if (0==stricmp(name,it->Name)) return u32(it-library.begin());
		return u32(-1);
	}
	Shader_xrLC*			Get		(LPCSTR name)
	{
		for (Shader_xrLCIt it=library.begin(); it!=library.end(); it++)
			if (0==stricmp(name,it->Name)) return &(*it);
		return NULL;
	}
	Shader_xrLC*			Get		(int id)
	{
		return &library[id];
	}
	Shader_xrLC*			Append	(Shader_xrLC* parent=0)
	{
		library.push_back(parent?Shader_xrLC(*parent):Shader_xrLC());
		return &library.back();
	}
	void					Remove	(LPCSTR name)
	{
		for (Shader_xrLCIt it=library.begin(); it!=library.end(); it++)
			if (0==stricmp(name,it->Name)){
            	library.erase(it);
                break;
            }
	}
	void					Remove	(int id)
	{
		library.erase(library.begin()+id);
	}
	Shader_xrLCVec&			Library	(){return library;}
};
#endif
