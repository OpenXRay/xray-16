//---------------------------------------------------------------------------
#ifndef SHToolsInterfaceH
#define SHToolsInterfaceH

// refs

enum EToolsID
{
    aeFirstTool = 0,
    aeEngine = 0,
    aeCompiler,
    aeMtl,
    aeMtlPair,
    aeSoundEnv,
    aeMaxTools
};

struct ISHInit
{
    EToolsID tools_id;
    UIItemListForm *m_Items;
    UIPropertiesForm *m_ItemProps;
    UIPropertiesForm *m_PreviewProps;
    ISHInit() { ZeroMemory(this, sizeof(ISHInit)); }
    ISHInit(EToolsID id, UIItemListForm *il, UIPropertiesForm *ip, UIPropertiesForm *pp)
    {
        tools_id = id;
        m_Items = il;
        m_ItemProps = ip;
        m_PreviewProps = pp;
    }
};
class ISHTools
{
protected:
    typedef ISHTools inherited;

    ISHInit Ext;

    BOOL m_bModified;

    BOOL m_bLockUpdate; // ���� ������� ������ ���������������  Update____From___()

    xr_string m_LastSelection;

public:
    ListItem *m_CurrentItem;

public:
    void ViewSetCurrentItem(LPCSTR full_name);
    xr_string ViewGetCurrentItem(bool bFolderOnly);
    // UIItemListForm::Node			ViewGetCurrentItem();
public:
    virtual void AppendItem(LPCSTR folder_name, LPCSTR parent = 0) = 0;
    virtual void FillItemList() = 0;

public:
    ISHTools(const ISHInit &init);
    virtual ~ISHTools() { ; }

    EToolsID ID() { return Ext.tools_id; }
    xr_string SelectedName();
    void RemoveCurrent();
    void RenameCurrent();

    virtual LPCSTR ToolsName() = 0;

    virtual void Reload() = 0;
    virtual void Load() = 0;
    virtual bool Save() = 0;

    bool IsModified() { return m_bModified; }
    virtual bool IfModified();
    virtual void Modified();

    virtual bool OnCreate() = 0;
    virtual void OnDestroy() = 0;
    virtual void OnActivate() = 0;
    virtual void OnDeactivate() = 0;

    // misc
    virtual void ResetCurrentItem() = 0;
    virtual void SetCurrentItem(LPCSTR name, bool bView) = 0;
    virtual void ApplyChanges(bool bForced = false) = 0;

    virtual void RealUpdateProperties() = 0;
    virtual void RealUpdateList() = 0;

    virtual void OnFrame() = 0;
    virtual void OnRender() = 0;

    virtual void OnDeviceCreate() = 0;
    virtual void OnDeviceDestroy() = 0;

    virtual void ZoomObject(bool bOnlySel);
    virtual void OnShowHint(AStringVec &ss) { ; }
    virtual void OnDrawUI() {}
    virtual void OnCloneItem(LPCSTR parent_path, LPCSTR new_full_name);
    virtual void OnCreateItem(LPCSTR path);
};
//---------------------------------------------------------------------------
#endif
