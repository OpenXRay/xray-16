#pragma once
#include "UIDragDropListEx.h"
#include "xrServerEntities/inventory_space.h"
class CInventoryOwner;

class CUIDragDropReferenceList : public CUIDragDropListEx
{
private:
    typedef CUIDragDropListEx inherited;
    typedef xr_vector<CUIStatic*> ITEMS_REFERENCES_VEC;
    typedef ITEMS_REFERENCES_VEC::iterator ITEMS_REFERENCES_VEC_IT;
    ITEMS_REFERENCES_VEC m_references;
    xr_vector<CUITextWnd*> m_labels;
    pcstr m_translation_id;

public:
    CUIDragDropReferenceList();
    virtual ~CUIDragDropReferenceList();
    virtual void SetItem(CUICellItem* itm);
    virtual void SetItem(CUICellItem* itm, Fvector2 abs_pos);
    virtual void SetItem(CUICellItem* itm, Ivector2 cell_pos);
    virtual CUICellItem* RemoveItem(CUICellItem* itm, bool force_root);

    void Initialize(pcstr labelSection = nullptr, pcstr translationId = nullptr, CUIXml* uiXml = nullptr);
    CUICellContainer* GetContainer() { return m_container; };
    void LoadItemTexture(LPCSTR section, Ivector2 cell_pos);
    void ReloadReferences(CInventoryOwner* pActor);
    void UpdateLabels();

    virtual void __stdcall OnItemDBClick(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemDrop(CUIWindow* w, void* pData);
};
