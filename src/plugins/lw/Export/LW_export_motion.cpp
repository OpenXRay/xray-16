#include "stdafx.h"
#include "xrCore/FileSystem.h"
#include "xrCore/FS.h"
#include "utils/LWO/envelope.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrCore/Animation/Motion.hpp"
#include "Globals.hpp"
#include "scenscan\objectdb.h"
#include "OMotionLW.hpp"

static OMotionLW* m_Motion;

void SelectedCount(LWItemType type, int& sel_obj_count, LWItemID& last_sel_obj)
{
    LWItemID object;
    object = g_iteminfo->first(type, NULL);
    while (object)
    {
        if (g_intinfo->itemFlags(object) & LWITEMF_SELECTED)
        {
            last_sel_obj = object;
            sel_obj_count++;
        }
        object = g_iteminfo->next(object);
    }
}

extern "C" {
//-----------------------------------------------------------------------------------------
void __cdecl SaveObjectMotion(GlobalFunc* global)
{
    Core.Initialize("XRayPlugin", nullptr, LogCallback(ELogCallback, nullptr), FALSE, nullptr, true);
    FS._initialize(CLocatorAPI::flScanAppRoot, NULL, "xray_path.ltx");
    // get bone ID
    bool bErr = false;

    string_path buf = "";

    EFS.GetSaveName("$omotion$", buf);
    if (buf[0])
    {
        int sel_obj_cnt = 0;
        LWItemID sel_object;

        SelectedCount(LWI_OBJECT, sel_obj_cnt, sel_object);
        SelectedCount(LWI_CAMERA, sel_obj_cnt, sel_object);
        SelectedCount(LWI_BONE, sel_obj_cnt, sel_object);
        SelectedCount(LWI_LIGHT, sel_obj_cnt, sel_object);

        if (sel_obj_cnt == 1)
        {
            string_path name;
            _splitpath(buf, 0, 0, name, 0);
            m_Motion = new OMotionLW();
            m_Motion->SetName(name);
            m_Motion->ParseObjectMotion(sel_object);
            m_Motion->SetParam(g_intinfo->previewStart, g_intinfo->previewEnd, (float)g_lwsi->framesPerSecond);
            m_Motion->SaveMotion(buf);
            g_msg->info("Export motion successful.", g_iteminfo->name(sel_object));
            xr_delete(m_Motion);
        }
        else
        {
            g_msg->error("Select one object and try again.", NULL);
        }
    }
    else
    {
        g_msg->error("Export failed. Empty name.", NULL);
    }
}
};
