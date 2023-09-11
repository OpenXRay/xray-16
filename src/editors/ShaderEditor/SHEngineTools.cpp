//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SHEngineTools.h"
#include "ui_shadermain.h"
#include "..\xrECore\Editor\EditMesh.h"
//------------------------------------------------------------------------------
class CCollapseBlender : public CParseBlender
{
public:
    virtual void Parse(CSHEngineTools *owner, DWORD type, LPCSTR key, LPVOID data)
    {
        switch (type)
        {
        case xrPID_MATRIX:
            owner->CollapseMatrix((LPSTR)data);
            break;
        case xrPID_CONSTANT:
            owner->CollapseConstant((LPSTR)data);
            break;
        };
    }
};

class CRefsBlender : public CParseBlender
{
public:
    virtual void Parse(CSHEngineTools *owner, DWORD type, LPCSTR key, LPVOID data)
    {
        switch (type)
        {
        case xrPID_MATRIX:
            owner->UpdateMatrixRefs((LPSTR)data);
            break;
        case xrPID_CONSTANT:
            owner->UpdateConstantRefs((LPSTR)data);
            break;
        };
    }
};

class CRemoveBlender : public CParseBlender
{
public:
    virtual void Parse(CSHEngineTools *owner, DWORD type, LPCSTR key, LPVOID data)
    {
        switch (type)
        {
        case xrPID_MATRIX:
            owner->RemoveMatrix((LPSTR)data);
            break;
        case xrPID_CONSTANT:
            owner->RemoveConstant((LPSTR)data);
            break;
        };
    }
};

static CCollapseBlender ST_CollapseBlender;
static CRefsBlender ST_UpdateBlenderRefs;
static CRemoveBlender ST_RemoveBlender;

//------------------------------------------------------------------------------
CSHEngineTools::CSHEngineTools(const ISHInit &init) : ISHTools(init)
{
    m_PreviewObjectType = pvoNone;
    m_PreviewObject = NULL;
    m_bCustomEditObject = FALSE;
    m_bFreezeUpdate = FALSE;
    m_CurrentBlender = 0;
    m_BlenderStream.clear();
    m_bNeedResetShaders = TRUE;
    m_RemoteRenBlender = FALSE;
    m_CreatingBlender = FALSE;
    m_SetCustomObject = FALSE;
    MCString.push_back("Custom...");
    MCString.push_back("$null");
    MCString.push_back("$base0");
    MCString.push_back("$base1");
    MCString.push_back("$base2");
    MCString.push_back("$base3");
    MCString.push_back("$base4");
    MCString.push_back("$base5");
    MCString.push_back("$base6");
    MCString.push_back("$base7");
}

CSHEngineTools::~CSHEngineTools()
{
    MCString.clear();
}

bool CSHEngineTools::OnCreate()
{
    IBlender::CreatePalette(m_TemplatePalette);
    Load();
    return true;
}

void CSHEngineTools::OnDestroy()
{
    Lib.RemoveEditObject(m_PreviewObject);
    // free palette
    for (TemplateIt it = m_TemplatePalette.begin(); it != m_TemplatePalette.end(); it++)
        xr_delete(*it);
    m_TemplatePalette.clear();

    ClearData();

    m_bModified = FALSE;
}

xr_token preview_obj_token[] = {
    {"None", pvoNone},
    {"Plane", pvoPlane},
    {"Box", pvoBox},
    {"Sphere", pvoSphere},
    {"Teapot", pvoTeapot},
    {"Custom...", pvoCustom},
    {0, 0}};

bool CSHEngineTools::OnPreviewObjectRefChange(PropValue *sender, u32 &new_val)
{
    LPCSTR fn = "";
    m_bCustomEditObject = false;
    switch (new_val)
    {
    case pvoPlane:
        fn = "editor\\ShaderTest_Plane";
        break;
    case pvoBox:
        fn = "editor\\ShaderTest_Box";
        break;
    case pvoSphere:
        fn = "editor\\ShaderTest_Sphere";
        break;
    case pvoTeapot:
        fn = "editor\\ShaderTest_Teapot";
        break;
    case pvoCustom:
    {
        m_SetCustomObject = TRUE;
        UIChooseForm::SelectItem(smObject, 1, 0, 0, 0, 0, 0, 0);
        return true;
    }
    break;
    }

    OnPreviewObjectRefChange(fn);
    return true;
}

void CSHEngineTools::OnPreviewObjectRefChange(const char *fn)
{
    string_path m_current;
    string_path m_new;
    xr_strcpy(m_current, fn);
    xr_strcpy(m_new, m_PreviewObject ? m_PreviewObject->GetName() : "");
    _strlwr_s(m_current);
    _strlwr_s(m_new);

    if (xr_strcmp(m_current, m_new) != 0)
    {
        Lib.RemoveEditObject(m_PreviewObject);
        m_PreviewObject = (fn && fn[0]) ? Lib.CreateEditObject(fn) : 0;
        ZoomObject(false);
        UpdateObjectShader();
        UI->RedrawScene();
    }
}

void CSHEngineTools::OnActivate()
{
    PropItemVec items;
    TokenValue<u32> *V = PHelper().CreateToken32(items, PrepareKey("Object", "Reference"), &m_PreviewObjectType, preview_obj_token);
    V->OnAfterEditEvent.bind(this, &CSHEngineTools::OnPreviewObjectRefChange);
    Ext.m_PreviewProps->AssignItems(items);
    // fill items*/
    FillItemList();
    /* Ext.m_Items->SetOnModifiedEvent		(TOnModifiedEvent(this,&CSHEngineTools::Modified));*/
    Ext.m_Items->SetOnItemRenameEvent(TOnItemRename(this, &CSHEngineTools::OnRenameItem));
    Ext.m_Items->SetOnItemRemoveEvent(TOnItemRemove(this, &CSHEngineTools::OnRemoveItem));
    Ext.m_Items->SetOnItemCloneEvent(TOnItemClone(this, &CSHEngineTools::OnCloneItem));
    Ext.m_Items->SetOnItemCreaetEvent(TOnItemCreate(this, &CSHEngineTools::OnCreateItem));

    inherited::OnActivate();
}
//---------------------------------------------------------------------------

void CSHEngineTools::OnDeactivate()
{
    inherited::OnDeactivate();
}

void FreeStr(LPSTR str)
{ 
    xr_free(str);
}

void CSHEngineTools::ClearData()
{
    // free constants, matrices, blenders
    // Blender
    for (BlenderPairIt b = m_Blenders.begin(); b != m_Blenders.end(); b++)
    {
        FreeStr(b->first);
        xr_delete(b->second);
    }
    m_Blenders.clear();

    // Matrices
    for (MatrixPairIt m = m_Matrices.begin(); m != m_Matrices.end(); m++)
    {
        FreeStr(m->first);
        xr_delete(m->second);
    }
    m_Matrices.clear();

    // Constants
    for (ConstantPairIt c = m_Constants.begin(); c != m_Constants.end(); c++)
    {
        LPSTR temp = (LPSTR)c->first;
        xr_free(temp);
        xr_delete(c->second);
    }
    m_Constants.clear();
}

void CSHEngineTools::OnFrame()
{
    if (m_bNeedResetShaders)
    {
        RealResetShaders();
        // ExecCommand		(COMMAND_UPDATE_LIST);
    }
    if (m_RemoteRenBlender)
    {
        RealRenameItem(m_RenBlenderOldName.c_str(), m_RenBlenderNewName.c_str());
        m_RemoteRenBlender = FALSE;
    }

    if (m_PreviewObject)
        m_PreviewObject->OnFrame();
    inherited::OnFrame();
    if (m_SetCustomObject)
    {
        bool change = true;
        shared_str result;
        if (UIChooseForm::GetResult(change, result))
        {
            if (change)
            {
                OnPreviewObjectRefChange(result.c_str());
            }
            else
            {

                m_PreviewObjectType = 0;
                OnPreviewObjectRefChange("");
            }
            m_SetCustomObject = FALSE;
        }
    }
    if (m_CreatingBlender)
    {
        bool change = true;
        shared_str result;
        if (UIChooseForm::GetResult(change, result))
        {
            if (change)
            {
                CLASS_ID cls_id = 0;
                for (TemplateIt it = m_TemplatePalette.begin(); it != m_TemplatePalette.end(); it++)
                {
                    if (0 == strcmp((*it)->getComment(), result.c_str()))
                    {
                        cls_id = (*it)->getDescription().CLS;
                        break;
                    }
                }
                R_ASSERT(cls_id);
                AppendItem(m_CreatingBlenderPath.c_str(), cls_id);
            }

            m_CreatingBlender = FALSE;
        }
    }
}

void CSHEngineTools::OnRender()
{
    if (m_PreviewObject)
        m_PreviewObject->RenderSingle(Fidentity);
}

void CSHEngineTools::ZoomObject(bool bOnlySel)
{
    if (m_PreviewObject)
    {
        Fbox bb = m_PreviewObject->GetBox();
        EDevice.m_Camera.ZoomExtents(bb);
    }
    else
    {
        ISHTools::ZoomObject(bOnlySel);
    }
}
//---------------------------------------------------------------------------

void CSHEngineTools::RealResetShaders()
{
    // disable props vis update
    m_bFreezeUpdate = TRUE;
    UpdateStreamFromObject();

    UpdateObjectShader();
    // save to temp file
    PrepareRender();
    // reset device shaders from temp file
    IReader data(m_RenderShaders.pointer(), m_RenderShaders.size());
    EDevice.Reset(&data, TRUE);
    // enable props vis update
    m_bFreezeUpdate = FALSE;
    m_bNeedResetShaders = FALSE;
}

void CSHEngineTools::FillItemList()
{
    ListItemsVec items;
    for (BlenderPairIt b = m_Blenders.begin(); b != m_Blenders.end(); b++)
        LHelper().CreateItem(items, b->first, 0);
    Ext.m_Items->AssignItems(items, m_LastSelection.size() ? m_LastSelection.c_str() : 0, false);
}

void CSHEngineTools::ApplyChanges(bool bForced)
{
    if (m_CurrentBlender && (Ext.m_ItemProps->IsModified() || bForced))
    {
        UpdateObjectFromStream();
        // Ext.m_ItemProps->ResetModified();
        ResetShaders(false); // required 'false' for matrix
    }
}

void CSHEngineTools::Reload()
{
    ResetCurrentItem();
    ClearData();
    Load();
    FillItemList();
    ResetShaders(false); // required 'false' for matrix
}

void CSHEngineTools::Load()
{
    string_path fn;
    FS.update_path(fn, _game_data_, "shaders.xr");

    m_bFreezeUpdate = TRUE;
    m_bLockUpdate = TRUE;

    if (FS.exist(fn))
    {
        IReader *F = FS.r_open(fn);
        char name[256];

        // Load constants
        {
            IReader *fs = F->open_chunk(0);
            while (fs && !fs->eof())
            {
                fs->r_stringZ(name, sizeof(name));
                CConstant *C = xr_new<CConstant>();
                C->Load(fs);
                m_Constants.insert(std::make_pair(xr_strdup(name), C));
            }
            fs->close();
        }

        // Load matrices
        {
            IReader *fs = F->open_chunk(1);
            while (fs && !fs->eof())
            {
                fs->r_stringZ(name, sizeof(name));
                CMatrix *M = xr_new<CMatrix>();
                M->Load(fs);
                m_Matrices.insert(std::make_pair(xr_strdup(name), M));
            }
            fs->close();
        }

        // Load blenders
        {
            IReader *fs = F->open_chunk(2);
            IReader *chunk = NULL;
            int chunk_id = 0;

            while ((chunk = fs->open_chunk(chunk_id)) != NULL)
            {
                CBlender_DESC desc;
                chunk->r(&desc, sizeof(desc));
                IBlender *B = IBlender::Create(desc.CLS);
                if (B)
                {
                    if (B->getDescription().version != desc.version)
                    {
                        Msg("! Version conflict in shader '%s'", desc.cName);
                    }
                    chunk->seek(0);
                    B->Load(*chunk, desc.version);

                    LPSTR blender_name = xr_strdup(desc.cName);
                    std::pair<BlenderPairIt, bool> I = m_Blenders.insert(std::make_pair(blender_name, B));
                    R_ASSERT2(I.second, "shader.xr - found duplicate name!!!");
                }
                chunk->close();
                chunk_id++;
            }
            fs->close();
        }
        FS.r_close(F);
        UpdateRefCounters();
        ResetCurrentItem();
    }
    else
    {
        ELog.DlgMsg(mtInformation, "Can't find file '%s'", fn);
    }
    m_bLockUpdate = FALSE;
    m_bFreezeUpdate = FALSE;
}

void CSHEngineTools::Save(CMemoryWriter &F)
{
    // Save constants
    {
        F.open_chunk(0);
        for (ConstantPairIt c = m_Constants.begin(); c != m_Constants.end(); c++)
        {
            F.w_stringZ(c->first);
            c->second->Save(&F);
        }
        F.close_chunk();
    }

    // Save matrices
    {
        F.open_chunk(1);
        for (MatrixPairIt m = m_Matrices.begin(); m != m_Matrices.end(); m++)
        {
            F.w_stringZ(m->first);
            m->second->Save(&F);
        }
        F.close_chunk();
    }

    // Save blenders
    {
        F.open_chunk(2);
        int chunk_id = 0;
        for (BlenderPairIt b = m_Blenders.begin(); b != m_Blenders.end(); b++)
        {
            F.open_chunk(chunk_id++);
            b->second->Save(F);
            F.close_chunk();
        }
        F.close_chunk();
    }
    // Save blender names
    {
        F.open_chunk(3);
        F.w_u32(m_Blenders.size());
        for (BlenderPairIt b = m_Blenders.begin(); b != m_Blenders.end(); b++)
            F.w_stringZ(b->first);
        F.close_chunk();
    }
}

bool CSHEngineTools::Save()
{
    // set name
    string_path fn;
    FS.update_path(fn, _game_data_, "shaders.xr");

    // collapse reference
    CollapseReferences();

    // save to stream
    CMemoryWriter F;

    Save(F);

    // save new file
    EFS.MarkFile(fn, false);
    bool bRes = F.save_to(fn);

    if (bRes)
    {
        m_bModified = FALSE;
        // Ext.m_ItemProps->ResetModified();
        //  restore shader
        ResetShaders();
    }
    return bRes;
}

void CSHEngineTools::PrepareRender()
{
    CollapseReferences();
    m_RenderShaders.clear();
    Save(m_RenderShaders);
}

IBlender *CSHEngineTools::FindItem(LPCSTR name)
{
    if (name && name[0])
    {
        LPSTR N = LPSTR(name);
        BlenderPairIt I = m_Blenders.find(N);
        if (I == m_Blenders.end())
            return 0;
        else
            return I->second;
    }
    else
        return 0;
}

CMatrix *CSHEngineTools::FindMatrix(LPCSTR name)
{
    R_ASSERT(name && name[0]);
    MatrixPairIt I = m_Matrices.find((LPSTR)name);
    if (I == m_Matrices.end())
        return 0;
    else
        return I->second;
}

CMatrix *CSHEngineTools::AppendMatrix(LPSTR name)
{
    CMatrix *M = FindMatrix(name);
    VERIFY(M);
    if (M->ref_count > 1)
    {
        M->ref_count--;
        strcpy(name, AppendMatrix(M, &M));
    }
    return M;
}

CConstant *CSHEngineTools::FindConstant(LPCSTR name)
{
    R_ASSERT(name && name[0]);
    ConstantPairIt I = m_Constants.find((LPSTR)name);
    if (I == m_Constants.end())
        return 0;
    else
        return I->second;
}

CConstant *CSHEngineTools::AppendConstant(LPSTR name)
{
    CConstant *C = FindConstant(name);
    if (C->ref_count > 1)
    {
        C->ref_count--;
        strcpy(name, AppendConstant(C, &C));
    }
    return C;
}

LPCSTR CSHEngineTools::GenerateMatrixName(LPSTR name)
{
    int cnt = 0;
    do
        sprintf(name, "%04x", cnt++);
    while (FindMatrix(name));
    return name;
}

LPCSTR CSHEngineTools::GenerateConstantName(LPSTR name)
{
    int cnt = 0;
    do
        sprintf(name, "%04x", cnt++);
    while (FindConstant(name));
    return name;
}

void CSHEngineTools::FillChooseTemplate(ChooseItemVec &items, void *param)
{
    for (TemplateIt it = m_TemplatePalette.begin(); it != m_TemplatePalette.end(); it++)
        items.push_back(SChooseItem((*it)->getComment(), ""));
}

void CSHEngineTools::AppendItem(LPCSTR path, LPCSTR parent_name)
{
    IBlender *parent = FindItem(parent_name);
    CLASS_ID cls_id;
    if (!parent)
    {
        UIChooseForm::SelectItem(smCustom, 1, 0, TOnChooseFillItems(this, &CSHEngineTools::FillChooseTemplate), 0, 0, 0, 0);
        m_CreatingBlender = TRUE;
        m_CreatingBlenderPath = path;
        return;
    }
    else
    {
        cls_id = parent->getDescription().CLS;
    }
    AppendItem(path, cls_id, parent);
}

void CSHEngineTools::RealRenameItem(LPCSTR old_full_name, LPCSTR new_full_name)
{
    ApplyChanges();
    LPSTR N = LPSTR(old_full_name);
    BlenderPairIt I = m_Blenders.find(N);
    R_ASSERT(I != m_Blenders.end());
    FreeStr(I->first);
    IBlender *B = I->second;
    m_Blenders.erase(I);
    // rename
    B->getDescription().Setup(new_full_name);
    std::pair<BlenderPairIt, bool> RES = m_Blenders.insert(std::make_pair(xr_strdup(new_full_name), B));
    R_ASSERT2(RES.second, "shader.xr - found duplicate name!!!");

    if (B == m_CurrentBlender)
        UpdateStreamFromObject();
    ApplyChanges();
    ResetShaders();
    if (B == m_CurrentBlender)
        SetCurrentItem(B->getName(), true);

    //.    RealUpdateProperties();
}

void CSHEngineTools::AddMatrixRef(LPSTR name)
{
    CMatrix *M = FindMatrix(name);
    R_ASSERT(M);
    M->ref_count++;
}

void CSHEngineTools::AddConstantRef(LPSTR name)
{
    CConstant *C = FindConstant(name);
    R_ASSERT(C);
    C->ref_count++;
}

LPCSTR CSHEngineTools::AppendConstant(CConstant *src, CConstant **dest)
{
    CConstant *C = xr_new<CConstant>();
    if (src)
        *C = *src;
    C->ref_count = 1;
    char name[128];
    std::pair<ConstantPairIt, bool> I = m_Constants.insert(std::make_pair(xr_strdup(GenerateConstantName(name)), C));
    VERIFY(I.second);
    if (dest)
        *dest = C;
    return I.first->first;
}

LPCSTR CSHEngineTools::AppendMatrix(CMatrix *src, CMatrix **dest)
{
    CMatrix *M = xr_new<CMatrix>();
    if (src)
        *M = *src;
    M->ref_count = 1;
    char name[128];
    std::pair<MatrixPairIt, bool> I = m_Matrices.insert(std::make_pair(xr_strdup(GenerateMatrixName(name)), M));
    VERIFY(I.second);
    if (dest)
        *dest = M;
    return I.first->first;
}

void CSHEngineTools::OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type)
{

    if (type == TYPE_OBJECT)
        RealRenameItem(old_full_name, new_full_name);
    m_bFreezeUpdate = TRUE;
}

void CSHEngineTools::OnRemoveItem(LPCSTR name, EItemType type)
{
    if (type == TYPE_OBJECT)
    {
        R_ASSERT(name && name[0]);
        IBlender *B = FindItem(name);
        R_ASSERT(B);
        // remove refs
        ParseBlender(B, ST_RemoveBlender);
        LPSTR N = LPSTR(name);
        BlenderPairIt I = m_Blenders.find(N);
        FreeStr(I->first);
        if (m_CurrentBlender == I->second)
        {
            m_CurrentBlender = 0;
            Tools->UpdateProperties(true);
        }
        xr_delete(I->second);

        m_Blenders.erase(I);

        ApplyChanges();
        ResetShaders();
    }
}

void CSHEngineTools::RemoveMatrix(LPCSTR name)
{
    if (*name == '$')
        return;
    R_ASSERT(name && name[0]);
    CMatrix *M = FindMatrix(name);
    VERIFY(M);
    M->ref_count--;
    if (M->ref_count == 0)
    {
        LPSTR N = LPSTR(name);
        MatrixPairIt I = m_Matrices.find(N);
        FreeStr(I->first);
        xr_delete(I->second);
        m_Matrices.erase(I);
    }
}

void CSHEngineTools::RemoveConstant(LPCSTR name)
{
    if (*name == '$')
        return;
    R_ASSERT(name && name[0]);
    CConstant *C = FindConstant(name);
    VERIFY(C);
    C->ref_count--;
    if (C->ref_count == 0)
    {
        LPSTR N = LPSTR(name);
        ConstantPairIt I = m_Constants.find(N);
        FreeStr(I->first);
        xr_delete(I->second);
        m_Constants.erase(I);
    }
}

void CSHEngineTools::UpdateStreamFromObject()
{
    m_BlenderStream.clear();
    if (m_CurrentBlender)
        m_CurrentBlender->Save(m_BlenderStream);
    // init properties
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}

void CSHEngineTools::UpdateObjectFromStream()
{
    if (m_CurrentBlender)
    {
        IReader data(m_BlenderStream.pointer(), m_BlenderStream.size());
        m_CurrentBlender->Load(data, m_CurrentBlender->getDescription().version);
    }
}

void CSHEngineTools::SetCurrentItem(LPCSTR name, bool bView)
{
    if (m_bLockUpdate)
        return;

    IBlender *B = FindItem(name);
    if (m_CurrentBlender != B)
    {
        m_CurrentBlender = B;
        UpdateStreamFromObject();
        // apply this shader to non custom object
        UpdateObjectShader();
        if (bView)
            ViewSetCurrentItem(name);
    }
}

void CSHEngineTools::ResetCurrentItem()
{
    m_CurrentBlender = 0;
    UpdateStreamFromObject();
}

void CSHEngineTools::CollapseMatrix(LPSTR name)
{
    if (*name == '$')
        return;
    R_ASSERT(name && name[0]);
    CMatrix *M = FindMatrix(name);
    VERIFY(M);
    M->ref_count--;
    for (MatrixPairIt m = m_OptMatrices.begin(); m != m_OptMatrices.end(); m++)
    {
        if (m->second->Similar(*M))
        {
            strcpy(name, m->first);
            m->second->ref_count++;
            return;
        }
    }
    // append new optimized matrix
    CMatrix *N = xr_new<CMatrix>(*M);
    N->ref_count = 1;
    m_OptMatrices.insert(std::make_pair(xr_strdup(name), N));
}

void CSHEngineTools::CollapseConstant(LPSTR name)
{
    if (*name == '$')
        return;
    R_ASSERT(name && name[0]);
    CConstant *C = FindConstant(name);
    VERIFY(C);
    C->ref_count--;
    for (ConstantPairIt c = m_OptConstants.begin(); c != m_OptConstants.end(); c++)
    {
        if (c->second->Similar(*C))
        {
            strcpy(name, c->first);
            c->second->ref_count++;
            return;
        }
    }
    // append opt constant
    CConstant *N = xr_new<CConstant>(*C);
    N->ref_count = 1;
    m_OptConstants.insert(std::make_pair(xr_strdup(name), N));
}

void CSHEngineTools::UpdateMatrixRefs(LPSTR name)
{
    if (*name == '$')
        return;
    R_ASSERT(name && name[0]);
    CMatrix *M = FindMatrix(name);
    R_ASSERT(M);
    M->ref_count++;
}

void CSHEngineTools::UpdateConstantRefs(LPSTR name)
{
    if (*name == '$')
        return;
    R_ASSERT(name && name[0]);
    CConstant *C = FindConstant(name);
    R_ASSERT(C);
    C->ref_count++;
}

void CSHEngineTools::ParseBlender(IBlender *B, CParseBlender &P)
{
    CMemoryWriter M;
    B->Save(M);

    IReader data(M.pointer(), M.size());
    data.advance(sizeof(CBlender_DESC));
    DWORD type;
    string256 key;

    while (!data.eof())
    {
        int sz = 0;
        type = data.r_u32();
        data.r_stringZ(key, sizeof(key));
        switch (type)
        {
        case xrPID_MARKER:
            break;
        case xrPID_MATRIX:
            sz = sizeof(string64);
            break;
        case xrPID_CONSTANT:
            sz = sizeof(string64);
            break;
        case xrPID_TEXTURE:
            sz = sizeof(string64);
            break;
        case xrPID_INTEGER:
            sz = sizeof(xrP_Integer);
            break;
        case xrPID_FLOAT:
            sz = sizeof(xrP_Float);
            break;
        case xrPID_BOOL:
            sz = sizeof(xrP_BOOL);
            break;
        case xrPID_TOKEN:
            sz = sizeof(xrP_TOKEN) + sizeof(xrP_TOKEN::Item) * (((xrP_TOKEN *)data.pointer())->Count);
            break;
        default:
            THROW2("xrPID_????");
        }
        P.Parse(this, type, key, data.pointer());
        data.advance(sz);
    }

    data.seek(0);
    B->Load(data, B->getDescription().version);
}

void CSHEngineTools::CollapseReferences()
{
    for (BlenderPairIt b = m_Blenders.begin(); b != m_Blenders.end(); b++)
        ParseBlender(b->second, ST_CollapseBlender);
    for (MatrixPairIt m = m_Matrices.begin(); m != m_Matrices.end(); m++)
    {
        VERIFY(m->second->dwReference == 0);
        FreeStr(m->first);
        xr_delete(m->second);
    }
    for (ConstantPairIt c = m_Constants.begin(); c != m_Constants.end(); c++)
    {
        VERIFY(c->second->dwReference == 0);
        FreeStr(c->first);
        xr_delete(c->second);
    }
    m_Matrices.clear();
    m_Constants.clear();
    m_Matrices = m_OptMatrices;
    m_Constants = m_OptConstants;
    m_OptConstants.clear();
    m_OptMatrices.clear();
}

void CSHEngineTools::UpdateRefCounters()
{
    for (BlenderPairIt b = m_Blenders.begin(); b != m_Blenders.end(); b++)
        ParseBlender(b->second, ST_UpdateBlenderRefs);
}

void CSHEngineTools::OnDeviceCreate()
{
    ResetShaders();
}

void CSHEngineTools::UpdateObjectShader()
{
    // apply this shader to non custom object
    CEditableObject *E = m_PreviewObject;
    if (E && !m_bCustomEditObject)
    {
        CSurface *surf = *E->FirstSurface();
        R_ASSERT(surf);
        /*
                u32 cnt = _GetItemCount(surf->_Texture());
                string512 	tex;
                string128 	elem;
                if (0==cnt){
                    strcpy	(elem,"$shadertest");
                }else{
                    _GetItem(surf->_Texture(),0,elem);
                }
                strcpy		(tex,surf->_Texture());
                for (int i=cnt; i<8; i++){ strcat(tex,","); strcat(tex,elem);}
                surf->SetTexture(tex);
        */
        if (m_CurrentBlender)
            surf->SetShader(m_CurrentBlender->getName());
        else
            surf->SetShader("editor\\wire");
        UI->RedrawScene();
        E->OnDeviceDestroy();
    }
}

void CSHEngineTools::OnShowHint(AStringVec &ss)
{
    if (m_PreviewObject)
    {
        float dist = UI->ZFar();
        SRayPickInfo pinf;
        if (m_PreviewObject->RayPick(dist, UI->m_CurrentRStart, UI->m_CurrentRDir, Fidentity, &pinf))
        {
            R_ASSERT(pinf.e_mesh);
            CSurface *surf = pinf.e_mesh->GetSurfaceByFaceID(pinf.inf.id);
            ss.push_back(xr_string("Surface: ") + xr_string(surf->_Name()));
            ss.push_back(xr_string("Texture: ") + xr_string(surf->_Texture()));
            ss.push_back(xr_string("Shader: ") + xr_string(surf->_ShaderName()));
            ss.push_back(xr_string("LC Shader: ") + xr_string(surf->_ShaderXRLCName()));
            ss.push_back(xr_string("Game Mtl: ") + xr_string(surf->_GameMtlName()));
            ss.push_back(xr_string("2 Sided: ") + xr_string(surf->m_Flags.is(CSurface::sf2Sided) ? "on" : "off"));
        }
    }
}

void CSHEngineTools::OnDrawUI()
{
    if (m_CreatingBlender || m_SetCustomObject)
    {
        UIChooseForm::Update();
    }
}

void CSHEngineTools::AppendItem(LPCSTR path, CLASS_ID cls_id, IBlender *parent)
{
    R_ASSERT2(cls_id, "Invalid CLASS_ID.");
    // append blender
    IBlender *B = IBlender::Create(cls_id);
    // append matrix& constant
    CMemoryWriter M;
    if (parent)
        parent->Save(M);
    else
        B->Save(M);
    // parse data
    IReader data(M.pointer(), M.size());
    data.advance(sizeof(CBlender_DESC));
    DWORD type;
    string256 key;

    while (!data.eof())
    {
        int sz = 0;
        type = data.r_u32();
        data.r_stringZ(key, sizeof(key));
        switch (type)
        {
        case xrPID_MARKER:
            break;
        case xrPID_MATRIX:
            sz = sizeof(string64);
            if (strcmp((LPSTR)data.pointer(), "$null") != 0)
            {
                if (!parent)
                    strcpy((LPSTR)data.pointer(), AppendMatrix());
                else
                    AddMatrixRef((LPSTR)data.pointer());
            }
            break;
        case xrPID_CONSTANT:
            sz = sizeof(string64);
            if (strcmp((LPSTR)data.pointer(), "$null") != 0)
            {
                if (!parent)
                    strcpy((LPSTR)data.pointer(), AppendConstant());
                else
                    AddConstantRef((LPSTR)data.pointer());
            }
            break;
        case xrPID_TEXTURE:
            sz = sizeof(string64);
            break;
        case xrPID_INTEGER:
            sz = sizeof(xrP_Integer);
            break;
        case xrPID_FLOAT:
            sz = sizeof(xrP_Float);
            break;
        case xrPID_BOOL:
            sz = sizeof(xrP_BOOL);
            break;
        case xrPID_TOKEN:
            sz = sizeof(xrP_TOKEN) + sizeof(xrP_TOKEN::Item) * (((xrP_TOKEN *)data.pointer())->Count);
            break;
        default:
            THROW2("xrPID_????");
        }
        data.advance(sz);
    }
    data.seek(0);
    B->Load(data, B->getDescription().version);
    // set name
    m_LastSelection = path;
    B->getDescription().Setup(m_LastSelection.c_str());
    // insert blender
    std::pair<BlenderPairIt, bool> I = m_Blenders.insert(std::make_pair(xr_strdup(m_LastSelection.c_str()), B));
    R_ASSERT2(I.second, "shader.xr - found duplicate name!!!");
    // insert to TreeView
    ExecCommand(COMMAND_UPDATE_LIST);
    ExecCommand(COMMAND_UPDATE_PROPERTIES);

    ResetShaders(true);
    Modified();
}
