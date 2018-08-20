#include "stdafx.h"
#include "editors/ECore/Editor/editobject.h"
#include "xrCore/FileSystem.h"
#include "xrCore/FS.h"
#include "xrCore/Animation/Bone.hpp"
#include "Globals.hpp"

class LWBoneParser final
{
public:
    LWBoneParser() = delete;

    static void LWBoneParser::Parse(CBone& bone, LWItemID boneId)
    {
        LWItemID P = g_iteminfo->parent(boneId);
        if (g_iteminfo->type(P) == LWI_BONE)
            bone.SetParentName(g_iteminfo->name(P));
        LWDVector vec;
        g_boneinfo->restParam(boneId, LWIP_POSITION, vec);
        bone.rest_offset.set((float)vec[0], (float)vec[1], (float)vec[2]);
        g_boneinfo->restParam(boneId, LWIP_ROTATION, vec);
        bone.rest_rotate.set((float)vec[1], (float)vec[0], (float)vec[2]);
        bone.rest_length = (float)g_boneinfo->restLength(boneId);
        bone.SetWMap(g_boneinfo->weightMap(boneId));
    }
};

static BoneVec* m_LWBones = 0;

static void AppendBone(LWItemID bone)
{
    m_LWBones->push_back(new CBone());
    CBone& B = *m_LWBones->back();
    B.SetName(g_iteminfo->name(bone));
    LWBoneParser::Parse(B, bone);
}

static void RecurseBone(LWItemID parent)
{
    LWItemID bone = g_iteminfo->firstChild(parent);
    while (bone != LWITEM_NULL)
    {
        if (g_iteminfo->type(bone) == LWI_BONE)
        {
            AppendBone(bone);
            RecurseBone(bone);
        }
        bone = g_iteminfo->nextChild(parent, bone);
    }
}

static bool ParseObjectBones(LWItemID object, int& obj_cnt)
{
    LWItemID bone, parent;
    bone = g_iteminfo->first(LWI_BONE, object);

    if (!bone)
    {
        g_msg->error("Can't find bone.", 0);
        return false;
    }

    while (true)
    {
        parent = g_iteminfo->parent(bone);
        if (!parent)
        {
            g_msg->error("Can't find root bone.", 0);
            return false;
        }
        if (g_iteminfo->type(parent) != LWI_BONE)
            break;
        else
            bone = parent;
    }

    if (bone)
    {
        if (obj_cnt > 0)
        {
            g_msg->error("Can't support multiple objects.", 0);
            return false;
        }
        AppendBone(bone);
        RecurseBone(bone);

        obj_cnt++;
    }
    return true;
}

extern void ReplaceSpaceAndLowerCase(shared_str& s);

static void RefineBoneNames(CEditableObject* obj)
{
    for (CBone* bone : obj->Bones())
    {
        shared_str name = bone->Name();
        shared_str parentName = bone->ParentName();
        ReplaceSpaceAndLowerCase(name);
        ReplaceSpaceAndLowerCase(parentName);
        bone->SetName(name.c_str());
        bone->SetParentName(parentName.c_str());
    }
}

extern "C" {
//-----------------------------------------------------------------------------------------
void __cdecl SaveObject(GlobalFunc* global)
{
    Core.Initialize("XRayPlugin", nullptr, LogCallback(ELogCallback, nullptr), FALSE, nullptr, true);
    FS._initialize(CLocatorAPI::flScanAppRoot, NULL, "xray_path.ltx");

    // get bone ID
    LWItemID object;
    bool bErr = true;

    string_path buf = "";
    object = g_iteminfo->first(LWI_OBJECT, NULL);
    int obj_cnt = 0;
    bool bObjSel = false;

    while (object)
    {
        if (g_intinfo->itemFlags(object) & LWITEMF_SELECTED)
        {
            bObjSel = true;
            if (EFS.GetSaveName("$import$", buf, 0, 0))
            {
                bErr = false;

                char name[1024];
                _splitpath(buf, 0, 0, name, 0);

                CEditableObject* obj = new CEditableObject(name);
                obj->SetVersionToCurrent(TRUE, TRUE);

                // parse bone if exist
                bool bBoneExists = false;
                if (g_iteminfo->first(LWI_BONE, object))
                {
                    m_LWBones = &obj->Bones();
                    bBoneExists = true;
                    if (!ParseObjectBones(object, obj_cnt))
                        bErr = true;
                    if (bErr)
                    {
                        // default bone part
                        obj->BoneParts().push_back(SBonePart());
                        SBonePart& BP = obj->BoneParts().back();
                        BP.alias = "default";
                        for (int b_i = 0; b_i < (int)obj->Bones().size(); b_i++)
                            BP.bones.push_back(obj->Bones()[b_i]->Name());
                    }
                }
                if (!bErr)
                {
                    LPCSTR lwo_nm = g_objinfo->filename(object);
                    { // append path
                        string_path path, dr, di;
                        _splitpath(lwo_nm, dr, di, 0, 0);
                        strconcat(sizeof(path), path, dr, di);
                        if (!FS.path_exist(path))
                            FS.append_path(path, path, 0, FALSE);
                    }
                    if (FS.exist(lwo_nm))
                    {
                        if (!obj->ImportLWO(lwo_nm, false))
                            bErr = true;
                        else
                        {
                            obj->m_Flags.set(CEditableObject::eoDynamic, TRUE);
                            obj->Optimize();
                            RefineBoneNames(obj);
                            obj->Save(buf);
                        }
                    }
                    else
                        bErr = true;
                }
                // перенести выше или проверить не перетерает ли инфу о костях

                if (bErr)
                    g_msg->error("Export failed.", 0);
                else
                    g_msg->info("Export successful.", buf);
                bErr = false;

                xr_delete(obj);
                m_LWBones = 0;
                //				freeObjectDB(odb);
                break;
            }
        }
        object = g_iteminfo->next(object);
    }
    if (!bObjSel)
        g_msg->error("Select object at first.", 0);
    else if (bErr)
        g_msg->error("Export failed.", 0);
}
//-----------------------------------------------------------------------------------------
};
