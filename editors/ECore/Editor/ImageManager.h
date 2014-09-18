//---------------------------------------------------------------------------
#ifndef ImageManagerH
#define ImageManagerH

#include "../xrEProps/folderlib.h"
#include "../Layers/xrRender/etextureparams.h"
#include "../ECore/Editor/EThumbnail.h"
class CEditableObject;

struct SSimpleImage{
	shared_str	name;
    DEFINE_VECTOR(U32Vec,DATAVec,DATAIt);
	DATAVec		layers;
    u32			w,h,a;
    u32			tag;
    int 		LongestEdge()const 	{return (w>h)?w:h;}
    int			Area()const			{return w*h;}
};
IC bool operator == (const SSimpleImage& a, shared_str nm){return a.name==nm;}
IC bool operator < (const SSimpleImage& a, const SSimpleImage& b){return a.name<b.name;}
DEFINE_VECTOR	(SSimpleImage,SSimpleImageVec,SSimpleImageVecIt);

class ECORE_API CImageManager{
    bool		MakeGameTexture		(ETextureThumbnail* THM, LPCSTR game_name, u32* data);
public:
    static void		MakeThumbnailImage	(ETextureThumbnail* THM, u32* data, u32 w, u32 h, u32 a);
public:
				CImageManager		(){;}
				~CImageManager		(){;}
	// texture routines
    void __stdcall 	RemoveTexture	(LPCSTR fname, EItemType type, bool& res);
    BOOL		CheckCompliance		(LPCSTR fname, int& compl);
    void		CheckCompliance		(FS_FileSet& files, FS_FileSet& compl);
    int			GetTextures			(FS_FileSet& files, BOOL bFolder=FALSE);
    int			GetTexturesRaw		(FS_FileSet& files, BOOL bFolder=FALSE);
//	int			GetServerModifiedTextures(CLocatorAPI::files_query& files);
	int 		GetLocalNewTextures	(FS_FileSet& files);
	void		SafeCopyLocalToServer(FS_FileSet& files);

	void		SynchronizeTextures	(bool sync_thm, bool sync_game, bool bForceGame, FS_FileSet* source_map, AStringVec* sync_list_without_extention, FS_FileSet* modif_map=0, bool bForceBaseAge=false);
    void 		SynchronizeTexture	(LPCSTR tex_name, int age);
//	void		ChangeFileAgeTo		(FS_FileSet* source_map, int age);
	// make/update routines
    bool		MakeGameTexture		(LPCSTR game_name, u32* data, const STextureParams& tp);
    void		CreateTextureThumbnail(ETextureThumbnail* THM, const AnsiString& src_name, LPCSTR path=0, bool bSetDefParam=true);
    BOOL		CreateOBJThumbnail	(LPCSTR tex_name, CEditableObject* obj, int age);
    void		CreateLODTexture	(CEditableObject* object, U32Vec& lod_pixels, U32Vec& nm_pixels, u32 tgt_w, u32 tgt_h, int samples, int quality);
    void		CreateLODTexture	(CEditableObject* object, LPCSTR tex_name, 	u32 tgt_w, u32 tgt_h, int samples, int age, int quality);
    void		CreateGameTexture	(LPCSTR src_name, ETextureThumbnail* thumb=0);
    bool		LoadTextureData		(LPCSTR src_name, U32Vec& data, u32& w, u32& h, int* age=0);

    // result 0-can't fit images, 1-ok, -1 can't load image 
    void		MergedTextureRemapUV(float& dest_u, float& dest_v, float src_u, float src_v, const Fvector2& offs, const Fvector2& scale, bool bRotate);
    int			CreateMergedTexture	(const RStringVec& src_names, LPCSTR dest_name, STextureParams::ETFormat fmt, int dest_width, int dest_height, Fvector2Vec& dest_offset, Fvector2Vec& dest_scale, boolVec& dest_rotate, U32Vec& remap);
    int			CreateMergedTexture	(const RStringVec& src_names, LPCSTR dest_name, STextureParams::ETFormat fmt, int dest_width_min, int dest_width_max, int dest_height_min, int dest_height_max, Fvector2Vec& dest_offset, Fvector2Vec& dest_scale, boolVec& dest_rotate, U32Vec& remap);
    int			CreateMergedTexture	(u32 layer_cnt, SSimpleImageVec& src_images, SSimpleImage& dst_image, int dest_width, int dest_height, Fvector2Vec& dest_offset, Fvector2Vec& dest_scale, boolVec& dest_rotate, U32Vec& remap);
	int			CreateMergedTexture	(u32 layer_cnt, SSimpleImageVec& src_images, SSimpleImage& dst_image, int dest_width_min, int dest_width_max, int dest_height_min, int dest_height_max, Fvector2Vec& dest_offset, Fvector2Vec& dest_scale, boolVec& dest_rotate, U32Vec& remap);
	void 		ApplyBorders		(U32Vec& tgt_data, u32 w, u32 h);

    EImageThumbnail* CreateThumbnail(LPCSTR src_name, ECustomThumbnail::THMType type, bool bLoad=true);

    void 		RefreshTextures		(AStringVec* modif);

    xr_string	UpdateFileName		(xr_string& fn);

    void		WriteAssociation	(CInifile* ltx_ini, LPCSTR base_name, const STextureParams& fmt);

    BOOL		CreateSmallerCubeMap(LPCSTR src_name, LPCSTR dst_name);
};

extern ECORE_API CImageManager ImageLib;
//---------------------------------------------------------------------------
#endif
