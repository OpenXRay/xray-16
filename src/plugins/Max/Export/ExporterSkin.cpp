#include "stdafx.h"
#pragma hdrstop

#include "MeshExpUtility.h"
#include "editors/ECore/Editor/EditObject.h"
#include "exporter.h"
//----------------------------------------------------------------------------------------------

BOOL CExporter::ExportSkin(INode* pNode, LPCSTR fname)
{
    m_Style = eExportSkin;
    //	ScanBones			(pNode);

    int i = U.ip->GetSelNodeCount();
    while (i--)
        ScanMesh(U.ip->GetSelNode(i));

    if (!Capture()) return FALSE;
    if (m_bHasError) return FALSE;
    CEditableObject* OBJECT = new CEditableObject("object");
    OBJECT->SetVersionToCurrent(TRUE, TRUE);
    if (!OBJECT->ImportMAXSkeleton(this)) {
        xr_delete(OBJECT);
        return FALSE;
    }
    OBJECT->Optimize();
    OBJECT->Save(fname);
    xr_delete(OBJECT);
    return TRUE;
};
