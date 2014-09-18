#include "stdafx.h"
#pragma hdrstop

#include "EThumbnail.h"
#ifndef XR_EPROPS_EXPORTS
	#include "ImageManager.h"
#endif
#pragma package(smart_init)

//------------------------------------------------------------------------------
#define THM_TEXTURE_VERSION				0x0012
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
ETextureThumbnail::ETextureThumbnail(LPCSTR src_name, bool bLoad):EImageThumbnail(src_name,ETTexture)
{
    if(!strchr(src_name,'\\'))
    {
      xr_string _name                     	= src_name;
      ImageLib.UpdateFileName             	(_name);
      m_Name                              	= _name.c_str();
      m_Name 	                			= ChangeFileExt(m_Name,".thm");
     }
    m_bValid                            	= false;
    if (bLoad)
#ifdef XR_EPROPS_EXPORTS
    	Load();
#else
		if (!Load())
                {
                 ImageLib.CreateTextureThumbnail(this,src_name);
                 }
#endif
}
//------------------------------------------------------------------------------

ETextureThumbnail::~ETextureThumbnail()
{
	m_Pixels.clear();
}
//------------------------------------------------------------------------------

int ETextureThumbnail::MemoryUsage()
{
	int mem_usage = _Width()*_Height()*4;
    switch (m_TexParams.fmt){
    case STextureParams::tfDXT1:
    case STextureParams::tfADXT1: 	mem_usage/=6; break;
    case STextureParams::tfDXT3:
    case STextureParams::tfDXT5: 	mem_usage/=4; break;
    case STextureParams::tf4444:
    case STextureParams::tf1555:
    case STextureParams::tf565: 	mem_usage/=2; break;
    case STextureParams::tfRGBA:	break;
    }
    string_path fn;
    FS.update_path	(fn,_game_textures_,EFS.ChangeFileExt(m_Name.c_str(),".seq").c_str());
    if (FS.exist(fn))
    {
        string128		buffer;
        IReader* F		= FS.r_open(0,fn);
        F->r_string		(buffer,sizeof(buffer));
        int cnt = 0;
        while (!F->eof()){
            F->r_string(buffer,sizeof(buffer));
            cnt++;
        }
        FS.r_close		(F);
        mem_usage *= cnt?cnt:1;
    }
    return mem_usage;
}
//------------------------------------------------------------------------------

void ETextureThumbnail::CreateFromData(u32* p, u32 w, u32 h)
{
	EImageThumbnail::CreatePixels(p, w, h);
    m_TexParams.width = w;
    m_TexParams.height= h;
    m_TexParams.flags.set(STextureParams::flHasAlpha,FALSE);
}
//------------------------------------------------------------------------------

bool Surface_Load(LPCSTR full_name, U32Vec& data, u32& w, u32& h, u32& a);

bool ETextureThumbnail::Load(LPCSTR src_name, LPCSTR path)
{
    string_path fn;
    strcpy(fn,EFS.ChangeFileExt(src_name?src_name:m_Name.c_str(),".thm").c_str() );
    if (path)
        FS.update_path(fn,path,fn);
    else
    {
        FS.update_path                  (fn,_game_textures_,fn);
    }

    if (!FS.exist(fn)) return false;

    IReader* F 		= FS.r_open(fn);
    u16 version 	= 0;

    R_ASSERT(F->r_chunk(THM_CHUNK_VERSION,&version));
    if( version!=THM_TEXTURE_VERSION ){
		Msg			("!Thumbnail: Unsupported version.");
        return 		false;
    }

/*
    IReader* D 		= F->open_chunk(THM_CHUNK_DATA); R_ASSERT(D);
    m_Pixels.resize	(THUMB_SIZE);
    D->r		 (m_Pixels.begin(),THUMB_SIZE*sizeof(u32));
    D->close		();
*/

    R_ASSERT		(F->find_chunk(THM_CHUNK_TYPE));
    m_Type		= THMType(F->r_u32());
    R_ASSERT		(m_Type==ETTexture);

    m_TexParams.Load(*F);
    m_Age 			= FS.get_file_age(fn);

    FS.r_close		(F);
    SetValid            ();
    return true;
}
//------------------------------------------------------------------------------

void ETextureThumbnail::Save(int age, LPCSTR path)
{
	if (!Valid()) 	return;

    CMemoryWriter F;
	F.open_chunk	(THM_CHUNK_VERSION);
	F.w_u16			(THM_TEXTURE_VERSION);
	F.close_chunk	();

/*
	F.w_chunk		(THM_CHUNK_DATA | CFS_CompressMark,m_Pixels.begin(),m_Pixels.size()*sizeof(u32));
*/        

    F.open_chunk	(THM_CHUNK_TYPE);
    F.w_u32			(m_Type);
	F.close_chunk	();

	m_TexParams.Save(F);

    string_path		fn;
    if (path)
        FS.update_path(fn, path, m_Name.c_str());
    else
    	FS.update_path(fn, _game_textures_, m_Name.c_str());

    if (F.save_to(fn))
    {
	    FS.set_file_age	(fn,age?age:m_Age);
    }else{
        Log			("!Can't save thumbnail:",fn);
    }
}
//------------------------------------------------------------------------------

void ETextureThumbnail::FillProp(PropItemVec& items, PropValue::TOnChange on_type_change)
{
	m_TexParams.FillProp		(m_SrcName.c_str(),items,on_type_change);
}
//------------------------------------------------------------------------------

void ETextureThumbnail::FillInfo(PropItemVec& items)
{                                                                         
	STextureParams& F			= m_TexParams;
    PHelper().CreateCaption		(items, "Format",					get_token_name(tfmt_token,F.fmt));
    PHelper().CreateCaption		(items, "Type",						get_token_name(ttype_token,F.type));
    PHelper().CreateCaption		(items, "Width",					shared_str().printf("%d",_Width()));
    PHelper().CreateCaption		(items, "Height",					shared_str().printf("%d",_Height()));
    PHelper().CreateCaption		(items, "Alpha",					_Alpha()?"on":"off");
}

BOOL ETextureThumbnail::similar(ETextureThumbnail* thm1, xr_vector<AnsiString>& sel_params)
{
	BOOL res = m_TexParams.similar(thm1->m_TexParams, sel_params);
  /*
    if(res)
    {
		xr_vector<AnsiString>::iterator it = sel_params.begin();
		xr_vector<AnsiString>::iterator it_e = sel_params.end();

        for(;it!=it_e;++it)
        {
           const AnsiString& par_name = *it;
			if(par_name=="Format")
            {
            	res = (m_TexParams.fmt == thm1->m_TexParams.fmt);
            }else
			if(par_name=="Type")
            {
            	res = (m_TexParams.type == thm1->m_TexParams.type);
            }else
			if(par_name=="Width")
            {
            	res = (_Width()==thm1->_Width());
            }else
			if(par_name=="Height")
            {
            	res = (_Height()==thm1->_Height());
            }else
			if(par_name=="Alpha")
            {
            	res = (_Alpha()==thm1->_Alpha());
            }
           if(!res)
           	break;
        }
    }
*/
    return res;
}

LPCSTR ETextureThumbnail::FormatString()
{
    return m_TexParams.FormatString();
}
//------------------------------------------------------------------------------

void ETextureThumbnail::Draw(HDC hdc, const Irect& R)
{
     if(0==m_Pixels.size())
     {
        u32                 image_w, image_h, image_a;
        xr_string fn_img    = EFS.ChangeFileExt(m_Name.c_str(),".tga");
        string_path fn;
        FS.update_path       (fn,_textures_,fn_img.c_str());

        if( !FS.exist(fn) )
        {
          fn_img    = EFS.ChangeFileExt(m_Name.c_str(),".dds");
          FS.update_path(fn,_game_textures_,fn_img.c_str());

          if( !FS.exist( fn ) )
          {
               ELog.Msg(mtError,"Can't make preview for texture '%s'.",m_Name.c_str());
               return;
          }
        }

        U32Vec data;
        u32 w, h, a;
        if (!Surface_Load(fn,data,image_w,image_h,image_a))
        {
               ELog.Msg(mtError,"Can't make preview for texture '%s'.",m_Name.c_str());
               return;
        }
        ImageLib.MakeThumbnailImage(this,data.begin(),image_w,image_h,image_a);
     }

	if (Valid())
        {
        Irect r;
        r.x1 = R.x1+1; 
        r.y1 = R.y1+1;
        r.x2 = R.x2-1; 
        r.y2 = R.y2-1;
        if (_Width()!=_Height())	FHelper.FillRect(hdc,r,0x00000000);
        if (_Width()>_Height())
        {
                r.y2 -= r.height()-iFloor(r.height()*float(_Height())/float(_Width()));
        }else
        {
                r.x2 -= r.width()-iFloor(r.width()*float(_Width())/float(_Height()));
        }
        inherited::Draw(hdc,r);
    }
}

