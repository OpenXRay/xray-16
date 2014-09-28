//----------------------------------------------------
// file: CEditableObjectImport.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "EditObject.h"
//#include "lwo2.h"
#include "LW_SHADERDEF.h"
#include "EditMesh.h"

extern "C" {
#include "lwo2.h"
};

#ifdef _EDITOR
	#include "ResourceManager.h"
    
	extern "C" __declspec(dllimport) lwObject* LWOImportObject(char* filename);
	extern "C" __declspec(dllimport) void LWOCloseFile(lwObject* object);
#endif

#ifdef _LW_EXPORT
bool CEditableObject::ImportLWO(const char* fn, bool optimize)
{
    string512 fname;
    strcpy(fname, fn);
#ifdef _EDITOR
    lwObject* lwObj = LWOImportObject(fname);
#else
    u32 errId;
    int errPos;
    lwObject* lwObj = lwGetObject(fname, &errId, &errPos);
#endif
    if (!lwObj)
    {
        ELog.DlgMsg(mtError, "Can't import LWO object file!");
        return false;
    }
    ELog.Msg(mtInformation, "CEditableObject: import lwo %s...", fname);
    bool result = false; // assume fail
    // parse lwo object
    m_Meshes.reserve(lwObj->nlayers);
    m_Surfaces.reserve(lwObj->nsurfs);
    int surfaceId = 0;
    for (lwSurface* lwSurf = lwObj->surf; lwSurf; lwSurf = lwSurf->next)
    {
        lwSurf->alpha_mode = surfaceId; // save surface id for internal use
        auto surf = new CSurface();
        m_Surfaces.push_back(surf);
        surf->SetName(lwSurf->name && lwSurf->name[0] ? lwSurf->name : "Default");
        surf->m_Flags.set(CSurface::sf2Sided, lwSurf->sideflags == 3);
        AnsiString enName = "default", lcName = "default", gmName = "default";
        if (lwSurf->nshaders && !stricmp(lwSurf->shader->name, SH_PLUGIN_NAME))
        {
            auto shader = (XRShader*)lwSurf->shader->data;
            enName = shader->en_name;
            lcName = shader->lc_name;
            gmName = shader->gm_name;
        }
        else
            ELog.Msg(mtError, "CEditableObject: Shader not found on surface '%s'.", surf->_Name());
#ifdef _EDITOR
        if (!Device.Resources->_FindBlender(enName.c_str()))
        {
            ELog.Msg(mtError, "CEditableObject: Render shader '%s' - can't find in library.\n"
                "Using 'default' shader on surface '%s'.", enName.c_str(), surf->_Name());
            enName = "default";
        }
        if (!Device.ShaderXRLC.Get(lcName.c_str()))
        {
            ELog.Msg(mtError, "CEditableObject: Compiler shader '%s' - can't find in library.\n"
                "Using 'default' shader on surface '%s'.", lcName.c_str(), surf->_Name());
            lcName = "default";
        }
        if (!GMLib.GetMaterial(gmName.c_str()))
        {
            ELog.Msg(mtError, "CEditableObject: Game material '%s' - can't find in library.\n"
                "Using 'default' material on surface '%s'.", lcName.c_str(), surf->_Name());
            gmName = "default";
        }
#endif
        // fill texture layers
        u32 textureCount = 0;
        for (st_lwTexture* Itx = lwSurf->color.tex; Itx; Itx = Itx->next)
        {
            string1024 tname = "";
            textureCount++;
            int cidx = -1;
            if (Itx->type == ID_IMAP)
                cidx = Itx->param.imap.cindex;
            else
            {
                ELog.DlgMsg(mtError, "Import LWO (Surface '%s'): 'Texture' is not Image Map!", surf->_Name());
                goto importFailed;
            }
            if (cidx != -1)
            {
                // get textures
                for (st_lwClip* lwClip = lwObj->clip; lwClip; lwClip = lwClip->next)
                {
                    if (cidx == lwClip->index && lwClip->type == ID_STIL)
                    {
                        strcpy(tname, lwClip->source.still.name);
                        break;
                    }
                }
                if (tname[0] == 0)
                {
                    ELog.DlgMsg(mtError, "Import LWO (Surface '%s'): "
                        "'Texture' name is empty or non 'STIL' type!", surf->_Name());
                    goto importFailed;
                }
                string256 textureName;
                _splitpath(tname, nullptr, nullptr, textureName, nullptr);
                surf->SetTexture(EFS.AppendFolderToName(textureName, 256, 1, true));
                // get vmap refs
                surf->SetVMap(Itx->param.imap.vmap_name);
            }
        }
        if (!surf->_VMap() || *surf->_VMap() == 0)
        {
            ELog.DlgMsg(mtError, "Invalid surface '%s'. Empty VMap.", surf->_Name());
            goto importFailed;
        }
        if (!surf->_Texture() || *surf->_Texture() == 0)
        {
            ELog.DlgMsg(mtError, "Can't create shader. Invalid surface '%s'. Textures empty.", surf->_Name());
            goto importFailed;
        }
        if (enName.c_str() == 0)
        {
            ELog.DlgMsg(mtError, "Can't create shader. Invalid surface '%s'. Shader empty.", surf->_Name());
            goto importFailed;
        }
        surf->SetShader(enName.c_str());
        surf->SetShaderXRLC(lcName.c_str());
        surf->SetGameMtl(gmName.c_str());
        surf->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | textureCount << D3DFVF_TEXCOUNT_SHIFT);
        surfaceId++;
    }
    // process mesh layers
    for (st_lwLayer* lwLayer = lwObj->layer; lwLayer; lwLayer = lwLayer->next)
    {
        auto mesh = new CEditableMesh(this);
        m_Meshes.push_back(mesh);
        mesh->SetName(lwLayer->name ? lwLayer->name : "");
        auto bbox = lwLayer->bbox;
        mesh->m_Box.set(bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);
        // parse mesh(lwo-layer) data
        if (lwLayer->nvmaps == 0)
        {
            ELog.DlgMsg(mtError, "Import LWO: Mesh layer must contain UV map!");
            goto importFailed;
        }
        // XXX nitrocaster: incompatible with multithreaded import
        static xr_map<void*, int> vmIndices;
        vmIndices.clear();
        int vmIndex = 0;
        for (st_lwVMap* lwVmap = lwLayer->vmap; lwVmap; lwVmap = lwVmap->next)
        {
            switch (lwVmap->type)
            {
            case ID_TXUV:
            {
                if (lwVmap->dim != 2)
                {
                    ELog.DlgMsg(mtError, "Import LWO: 'UV Map' must be equal to 2!");
                    goto importFailed;
                }
                mesh->m_VMaps.push_back(new st_VMap(lwVmap->name, vmtUV, !!lwVmap->perpoly));
                st_VMap* vmap = mesh->m_VMaps.back();
                vmap->copyfrom(*lwVmap->val, lwVmap->nverts);
                // flip uv
                for (int uvIndex = 0; uvIndex < vmap->size(); uvIndex++)
                {
                    Fvector2& uv = vmap->getUV(uvIndex);
                    uv.y = 1.f - uv.y;
                }
                vmIndices[lwVmap] = vmIndex++;
                break;
            }
            case ID_WGHT:
            {
                if (lwVmap->dim != 1)
                {
                    ELog.DlgMsg(mtError, "Import LWO: 'Weight' must be equal to 1!");
                    goto importFailed;
                }
                mesh->m_VMaps.push_back(new st_VMap(lwVmap->name, vmtWeight, false));
                st_VMap* vmap = mesh->m_VMaps.back();
                vmap->copyfrom(*lwVmap->val, lwVmap->nverts);
                vmIndices[lwVmap] = vmIndex++;
                break;
            }
            case ID_PICK: ELog.Msg(mtError, "Found 'PICK' VMAP. Import failed."); goto importFailed;
            case ID_MNVW: ELog.Msg(mtError, "Found 'MNVW' VMAP. Import failed."); goto importFailed;
            case ID_MORF: ELog.Msg(mtError, "Found 'MORF' VMAP. Import failed."); goto importFailed;
            case ID_SPOT: ELog.Msg(mtError, "Found 'SPOT' VMAP. Import failed."); goto importFailed;
            case ID_RGB: ELog.Msg(mtError, "Found 'RGB' VMAP. Import failed."); goto importFailed;
            case ID_RGBA: ELog.Msg(mtError, "Found 'RGBA' VMAP. Import failed."); goto importFailed;
            }
        }
        // points
        mesh->m_VertCount = lwLayer->point.count;
        mesh->m_Vertices = xr_alloc<Fvector>(mesh->m_VertCount);
        for (int i = 0; i < lwLayer->point.count; i++)
        {
            st_lwPoint& lwPoint = lwLayer->point.pt[i];
            Fvector& vertex = mesh->m_Vertices[i];
            vertex.set(lwPoint.pos);
        }
        // polygons
        mesh->m_FaceCount = lwLayer->polygon.count;
        mesh->m_Faces = xr_alloc<st_Face>(mesh->m_FaceCount);
        mesh->m_SmoothGroups = xr_alloc<u32>(mesh->m_FaceCount);
        memset(mesh->m_SmoothGroups, u32(-1), mesh->m_FaceCount);
        mesh->m_VMRefs.reserve(lwLayer->polygon.count*3);
        xr_vector<int> surfIds(lwLayer->polygon.count);
        for (int i = 0; i < lwLayer->polygon.count; i++)
        {
            st_Face& face = mesh->m_Faces[i];
            st_lwPolygon& lwPoly = lwLayer->polygon.pol[i];
            if (lwPoly.nverts != 3)
            {
                ELog.DlgMsg(mtError, "Import LWO: Face must contain only 3 vertices!");
                goto importFailed;
            }
            for (int fvIndex = 0; fvIndex < 3; fvIndex++)
            {
                st_lwPolVert& lwFaceVert = lwPoly.v[fvIndex];
                st_FaceVert& faceVert = face.pv[fvIndex];
                faceVert.pindex = lwFaceVert.index;
                mesh->m_VMRefs.push_back(st_VMapPtLst());
                st_VMapPtLst& vmPointList = mesh->m_VMRefs.back();
                faceVert.vmref = mesh->m_VMRefs.size()-1;
                // parse uv-map                        
                st_lwPoint& lwPoint = lwLayer->point.pt[faceVert.pindex];
                if (lwFaceVert.nvmaps == 0 && lwPoint.nvmaps == 0)
                {
                    ELog.DlgMsg(mtError, "Found mesh without UV's!");
                    goto importFailed;
                }
                xr_vector<st_VMapPt> vmPoints;
                AStringVec names;
                // process polys
                for (int j = 0; j < lwFaceVert.nvmaps; j++)
                {
                    if (lwFaceVert.vm[j].vmap->type != ID_TXUV)
                        continue;
                    vmPoints.push_back(st_VMapPt());
                    st_VMapPt& pt = vmPoints.back();
                    pt.vmap_index = vmIndices[lwFaceVert.vm[j].vmap]; // VMap id
                    names.push_back(lwFaceVert.vm[j].vmap->name);
                    pt.index = lwFaceVert.vm[j].index;
                }
                // process points
                for (int j = 0; j < lwPoint.nvmaps; j++)
                {
                    if (lwPoint.vm[j].vmap->type != ID_TXUV)
                        continue;
                    if (std::find(names.begin(), names.end(), lwPoint.vm[j].vmap->name) != names.end())
                        continue;
                    vmPoints.push_back(st_VMapPt());
                    st_VMapPt& pt = vmPoints.back();
                    pt.vmap_index = vmIndices[lwPoint.vm[j].vmap]; // VMap id
                    pt.index = lwPoint.vm[j].index;
                }
                auto cmpFunc = [](const st_VMapPt& a, const st_VMapPt& b)
                { return a.vmap_index < b.vmap_index; };
                std::sort(vmPoints.begin(), vmPoints.end(), cmpFunc);
                // parse weight-map
                for (int j = 0; j < lwPoint.nvmaps; j++)
                {
                    if (lwPoint.vm[j].vmap->type != ID_WGHT)
                        continue;
                    vmPoints.push_back(st_VMapPt());
                    st_VMapPt& pt = vmPoints.back();
                    pt.vmap_index = vmIndices[lwPoint.vm[j].vmap]; // VMap id
                    pt.index = lwPoint.vm[j].index;
                }
                vmPointList.count = vmPoints.size();
                vmPointList.pts = xr_alloc<st_VMapPt>(vmPointList.count);
                memcpy(vmPointList.pts, &*vmPoints.begin(), vmPointList.count*sizeof(st_VMapPt));
            }
            // lwPoly.surf->alpha_mode stores reviously saved surface id
            surfIds[i] = lwPoly.surf->alpha_mode;
        }
        for (u32 polyId = 0; polyId < mesh->GetFCount(); polyId++)
            mesh->m_SurfFaces[m_Surfaces[surfIds[polyId]]].push_back(polyId);
        if (optimize)
            mesh->OptimizeMesh(false);
        mesh->RebuildVMaps();
    }
    result = true;
importFailed:
#ifdef _EDITOR
    LWOCloseFile(lwObj);
#else
    lwFreeObject(lwObj);
#endif
    if (result)
    {
        VerifyMeshNames();
        char* ext = strext(fname);
        m_LoadName = ext ? strcpy(ext, ".object") : strcat(fname, ".object");
    }
    else
        ELog.DlgMsg(mtError, "Can't parse LWO object.");
    return result;
}
#endif
