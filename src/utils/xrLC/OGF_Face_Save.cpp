#include "stdafx.h"
#include "build.h"
#include "ogf_face.h"

#include "xrCore/fs.h"
#include "xrCore/FMesh.hpp"
#include "Common/OGF_GContainer_Vertices.hpp"

using namespace std;

extern u16 RegisterShader(LPCSTR T);
extern void geom_batch_average(u32 verts, u32 faces);

void OGF::Save(IWriter& fs)
{
    OGF_Base::Save(fs);

    // clMsg			("* %d faces",faces.size());
    geom_batch_average((u32)data.vertices.size(), (u32)data.faces.size());

    // Texture & shader
    std::string Tname;
    for (u32 i = 0; i < textures.size(); i++)
    {
        if (!Tname.empty())
            Tname += ',';
        string256 t;
        xr_strcpy(t, *textures[i].name);
        if (strchr(t, '.'))
            *strchr(t, '.') = 0;
        Tname += t;
    }
    string1024 sid;
    strconcat(sizeof(sid), sid, pBuild->shader_render[pBuild->materials()[material].shader].name, "/", Tname.c_str());

    // Create header
    ogf_header H;
    H.format_version = xrOGF_FormatVersion;
    H.type = data.m_SWI.count ? MT_PROGRESSIVE : MT_NORMAL;
    H.shader_id = RegisterShader(sid);
    H.bb.min = bbox.vMin;
    H.bb.max = bbox.vMax;
    H.bs.c = C;
    H.bs.r = R;

    // Vertices
    const Shader_xrLC* SH = pBuild->shaders().Get(pBuild->materials()[material].reserved);
    bool bVertexColors = (SH->flags.bLIGHT_Vertex);

    switch (H.type)
    {
    case MT_NORMAL:
    case MT_PROGRESSIVE: Save_Normal_PM(fs, H, bVertexColors); break;
    }

    // Header
    fs.open_chunk(OGF_HEADER);
    fs.w(&H, sizeof(H));
    fs.close_chunk();
}

void OGF_Reference::Save(IWriter& fs)
{
    OGF_Base::Save(fs);

    // geom_batch_average	(vertices.size(),faces.size());	// don't use reference(s) as batch estimate

    // Texture & shader
    std::string Tname;
    for (u32 i = 0; i < textures.size(); i++)
    {
        if (!Tname.empty())
            Tname += ',';
        string256 t;
        xr_strcpy(t, *textures[i].name);
        if (strchr(t, '.'))
            *strchr(t, '.') = 0;
        Tname += t;
    }
    string1024 sid;
    strconcat(sizeof(sid), sid, pBuild->shader_render[pBuild->materials()[material].shader].name, "/", Tname.c_str());

    // Create header
    ogf_header H;
    H.format_version = xrOGF_FormatVersion;
    H.type = model->data.m_SWI.count ? MT_TREE_PM : MT_TREE_ST;
    H.shader_id = RegisterShader(sid);
    H.bb.min = bbox.vMin;
    H.bb.max = bbox.vMax;
    H.bs.c = C;
    H.bs.r = R;

    // Vertices
    fs.open_chunk(OGF_GCONTAINER);
    fs.w_u32(vb_id);
    fs.w_u32(vb_start);
    fs.w_u32((u32)model->data.vertices.size());

    fs.w_u32(ib_id);
    fs.w_u32(ib_start);
    fs.w_u32((u32)model->data.faces.size() * 3);
    fs.close_chunk();

    // Special
    fs.open_chunk(OGF_TREEDEF2);
    fs.w(&xform, sizeof(xform));
    fs.w(&c_scale, 5 * sizeof(float));
    fs.w(&c_bias, 5 * sizeof(float));
    fs.close_chunk();

    // Header
    fs.open_chunk(OGF_HEADER);
    fs.w(&H, sizeof(H));
    fs.close_chunk();

    // progressive
    if (H.type == MT_TREE_PM)
    {
        // SW
        fs.open_chunk(OGF_SWICONTAINER);
        fs.w_u32(sw_id);
        fs.close_chunk();
    }
}

void OGF::PreSave(u32 tree_id)
{
    // if (20==tree_id || 18==tree_id)	__asm int 3;	//.
    const Shader_xrLC* SH = pBuild->shaders().Get(pBuild->materials()[material].reserved);
    bool bVertexColored = (SH->flags.bLIGHT_Vertex);

    // X-vertices/faces
    if (fast_path_data.vertices.size() && fast_path_data.faces.size())
    {
        Logger.clMsg("%4d: v(%3d)/f(%3d)", tree_id, fast_path_data.vertices.size(), fast_path_data.faces.size());
        VDeclarator x_D;
        x_D.set(x_decl_vert);
        x_VB.Begin(x_D);
        for (itXV V = fast_path_data.vertices.begin(); V != fast_path_data.vertices.end(); ++V)
        {
            x_vert v(V->P);
            x_VB.Add(&v, sizeof(v));
        }
        x_VB.End(&fast_path_data.vb_id, &fast_path_data.vb_start);
        x_IB.Register((u16*)(&*fast_path_data.faces.begin()), (u16*)(&*fast_path_data.faces.end()),
            &fast_path_data.ib_id, &fast_path_data.ib_start);
    }

    // Vertices
    VDeclarator D;
    if (bVertexColored)
    {
        // vertex-colored
        D.set(r1_decl_vert);
        g_VB.Begin(D);
        for (itOGF_V V = data.vertices.begin(); V != data.vertices.end(); ++V)
        {
            r1v_vert v(V->P, V->N, V->T, V->B, V->Color, V->UV[0]);
            g_VB.Add(&v, sizeof(v));
        }
        g_VB.End(&data.vb_id, &data.vb_start);
    }
    else
    {
        // lmap-colored
        D.set(r1_decl_lmap);
        g_VB.Begin(D);
        for (itOGF_V V = data.vertices.begin(); V != data.vertices.end(); ++V)
        {
            r1v_lmap v(V->P, V->N, V->T, V->B, V->Color, V->UV[0], V->UV[1]);
            g_VB.Add(&v, sizeof(v));
        }
        g_VB.End(&data.vb_id, &data.vb_start);
    }

    // Faces
    g_IB.Register((u16*)(&*data.faces.begin()), (u16*)(&*data.faces.end()), &data.ib_id, &data.ib_start);
}

template <typename ogf_data_type>
void write_ogf_container(IWriter& fs, const ogf_data_type& ogf_cnt)
{
    fs.open_chunk(OGF_GCONTAINER);
    fs.w_u32(ogf_cnt.vb_id);
    fs.w_u32(ogf_cnt.vb_start);
    fs.w_u32((u32)ogf_cnt.vertices.size());

    fs.w_u32(ogf_cnt.ib_id);
    fs.w_u32(ogf_cnt.ib_start);
    fs.w_u32((u32)ogf_cnt.faces.size() * 3);
    fs.close_chunk();
}

template <typename ogf_data_type>
void read_ogf_container(IReader& fs_, const ogf_data_type& ogf_cnt)
{
    IReader& fs = *fs_.open_chunk(OGF_GCONTAINER);

    ogf_cnt.vb_id = fs.r_u32();
    ogf_cnt.vb_start = fs.r_u32();

    u32 vertises_size = fs.r_u32();
    // ogf_cnt.vertices.resize( vertises_size );

    ogf_cnt.ib_id = fs.r_u32();
    ogf_cnt.ib_start = fs.r_u32();
    u32 faces_size = fs.r_u32(); //(u32)ogf_cnt.faces.size()*3
    // ogf_cnt.faces.resize( vertises_size );
    // fs.close_chunk	( );
}

void write_ogf_swidata(IWriter& fs, const FSlideWindowItem& swi)
{
    fs.open_chunk(OGF_SWIDATA);
    fs.w_u32(swi.reserved[0]);
    fs.w_u32(swi.reserved[1]);
    fs.w_u32(swi.reserved[2]);
    fs.w_u32(swi.reserved[3]);
    fs.w_u32(swi.count);
    fs.w(swi.sw, swi.count * sizeof(FSlideWindow));
    fs.close_chunk();
}

void read_ogf_swidata(IReader& fs_, FSlideWindowItem& swi)
{
    IReader& fs = *fs_.open_chunk(OGF_SWIDATA);

    swi.reserved[0] = fs.r_u32();
    swi.reserved[1] = fs.r_u32();
    swi.reserved[2] = fs.r_u32();
    swi.reserved[3] = fs.r_u32();
    swi.count = fs.r_u32();
    VERIFY(!swi.sw);
    // swi.sw				=
    fs.r(swi.sw, swi.count * sizeof(FSlideWindow));
    // fs.close_chunk		();
}

void write_ogf_fastpath(IWriter& fs, const OGF& ogf, BOOL progresive)
{
    fs.open_chunk(OGF_FASTPATH);
    {
        // Vertices
        write_ogf_container(fs, ogf.fast_path_data);

        // progressive-data, if need it
        if (progresive) // H.type == MT_PROGRESSIVE
            write_ogf_swidata(fs, ogf.fast_path_data.m_SWI);
    }
    fs.close_chunk();
}

void OGF::Save_Normal_PM(IWriter& fs, ogf_header& H, BOOL bVertexColored)
{
    //	clMsg			("- saving: normal or clod");

    // Vertices
    write_ogf_container(fs, data);

    // progressive-data, if need it
    if (H.type == MT_PROGRESSIVE)
        write_ogf_swidata(fs, data.m_SWI); // SW

    // if has x-vertices/x-faces
    if (!fast_path_data.vertices.empty() && !fast_path_data.faces.empty())
        write_ogf_fastpath(fs, *this, H.type == MT_PROGRESSIVE);
}

void OGF::Load_Normal_PM(IReader& fs, ogf_header& H, BOOL bVertexColored) {}
