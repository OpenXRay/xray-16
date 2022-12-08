#pragma once
#ifndef xrEPropsH
#define xrEPropsH

#ifdef XR_EPROPS_EXPORTS
#define XR_EPROPS_API XR_EXPORT
#else
#ifndef XR_EPROPS_API
#define XR_EPROPS_API XR_IMPORT
#endif
#endif

#include "PropertiesListTypes.h"
#include "ItemListTypes.h"

enum EItemType
{
    TYPE_INVALID = -1,
    TYPE_FOLDER = 0,
    TYPE_OBJECT = 1
};

typedef fastdelegate::FastDelegate1<ListItemsVec&> TOnILItemsFocused;
typedef fastdelegate::FastDelegate0<> TOnILCloseEvent;
typedef fastdelegate::FastDelegate3<LPCSTR, LPCSTR, EItemType> TOnItemRename;
typedef fastdelegate::FastDelegate3<LPCSTR, EItemType, bool&> TOnItemRemove;
typedef fastdelegate::FastDelegate0<> TOnItemAfterRemove;
typedef fastdelegate::FastDelegate0<> TOnCloseEvent;
typedef fastdelegate::FastDelegate0<> TOnModifiedEvent;

#ifdef __BORLANDC__
#include "mxPlacemnt.hpp"
typedef fastdelegate::FastDelegate1<TElTreeItem*> TOnILItemFocused;
//---------------------------------------------------------------------------
void XR_EPROPS_API CheckWindowPos(TForm* form);
//---------------------------------------------------------------------------
#endif

//------------------------------------------------------------------------------
// Prepare Key
//------------------------------------------------------------------------------
IC xr_string FolderAppend(LPCSTR val)
{
    xr_string tmp = (val && val[0]) ? val : "";
    if (val && val[0])
        tmp += DELIMITER;
    return tmp;
}
IC shared_str PrepareKey(LPCSTR pref, LPCSTR key)
{
    R_ASSERT(key);
    return shared_str(xr_string(FolderAppend(pref) + key).c_str());
}
IC shared_str PrepareKey(LPCSTR pref0, LPCSTR pref1, LPCSTR key)
{
    R_ASSERT(key);
    return shared_str(xr_string(FolderAppend(pref0) + FolderAppend(pref1) + key).c_str());
}
IC shared_str PrepareKey(LPCSTR pref0, LPCSTR pref1, LPCSTR pref2, LPCSTR key)
{
    R_ASSERT(key);
    return shared_str(xr_string(FolderAppend(pref0) + FolderAppend(pref1) + FolderAppend(pref2) + key).c_str());
}
//------------------------------------------------------------------------------
// Properties
//------------------------------------------------------------------------------
class XR_EPROPS_API IPropHelper
{
public:
    virtual PropItem* FindItem(PropItemVec& items, shared_str key, EPropType type = PROP_UNDEF) = 0;

public:
    //------------------------------------------------------------------------------
    // predefind event routines
    virtual bool FvectorRDOnAfterEdit(PropValue* sender, Fvector& edit_val) = 0;
    virtual void FvectorRDOnBeforeEdit(PropValue* sender, Fvector& edit_val) = 0;
    virtual void FvectorRDOnDraw(PropValue* sender, xr_string& draw_val) = 0;
    virtual bool floatRDOnAfterEdit(PropValue* sender, float& edit_val) = 0;
    virtual void floatRDOnBeforeEdit(PropValue* sender, float& edit_val) = 0;
    virtual void floatRDOnDraw(PropValue* sender, xr_string& draw_val) = 0;
    // R-name edit
    virtual void NameBeforeEdit(PropValue* sender, shared_str& edit_val) = 0;
    virtual bool NameAfterEdit(PropValue* sender, shared_str& edit_val) = 0;
    virtual void NameDraw(PropValue* sender, xr_string& draw_val) = 0;
    // C-name edit
    virtual void CNameBeforeEdit(PropValue* sender, xr_string& edit_val) = 0;
    virtual bool CNameAfterEdit(PropValue* sender, xr_string& edit_val) = 0;
    virtual void CNameDraw(PropValue* sender, xr_string& draw_val) = 0;

public:
    virtual CaptionValue* CreateCaption(PropItemVec& items, shared_str key, shared_str val) = 0;
    virtual CanvasValue* CreateCanvas(PropItemVec& items, shared_str key, shared_str val, int height) = 0;
    virtual ButtonValue* CreateButton(
        PropItemVec& items, shared_str key, shared_str val, u32 flags, ButtonValue::TOnBtnClick onclick = nullptr) = 0;
    virtual ChooseValue* CreateChoose(PropItemVec& items, shared_str key, shared_str* val, u32 mode,
        LPCSTR path = nullptr, void* fill_param = nullptr, u32 sub_item_count = 1, u32 choose_flags = cfAllowNone) = 0;
    virtual S8Value* CreateS8(
        PropItemVec& items, shared_str key, s8* val, s8 mn = 0, s8 mx = 100, s8 inc = 1) = 0;
    virtual S16Value* CreateS16(
        PropItemVec& items, shared_str key, s16* val, s16 mn = 0, s16 mx = 100, s16 inc = 1) = 0;
    virtual S32Value* CreateS32(
        PropItemVec& items, shared_str key, s32* val, s32 mn = 0, s32 mx = 100, s32 inc = 1) = 0;
    virtual U8Value* CreateU8(
        PropItemVec& items, shared_str key, u8* val, u8 mn = 0, u8 mx = 100, u8 inc = 1) = 0;
    virtual U16Value* CreateU16(
        PropItemVec& items, shared_str key, u16* val, u16 mn = 0, u16 mx = 100, u16 inc = 1) = 0;
    virtual U32Value* CreateU32(
        PropItemVec& items, shared_str key, u32* val, u32 mn = 0, u32 mx = 100, u32 inc = 1) = 0;
    virtual FloatValue* CreateFloat(PropItemVec& items, shared_str key, float* val, float mn = 0.f,
        float mx = 1.f, float inc = 0.01f, int decim = 2) = 0;
    virtual BOOLValue* CreateBOOL(PropItemVec& items, shared_str key, BOOL* val) = 0;
    virtual VectorValue* CreateVector(PropItemVec& items, shared_str key, Fvector* val, float mn = 0.f,
        float mx = 1.f, float inc = 0.01f, int decim = 2) = 0;
    virtual Flag8Value* CreateFlag8(
        PropItemVec& items, shared_str key, Flags8* val, u8 mask, LPCSTR c0 = nullptr, LPCSTR c1 = nullptr, u32 flags = 0) = 0;
    virtual Flag16Value* CreateFlag16(
        PropItemVec& items, shared_str key, Flags16* val, u16 mask, LPCSTR c0 = nullptr, LPCSTR c1 = nullptr, u32 flags = 0) = 0;
    virtual Flag32Value* CreateFlag32(
        PropItemVec& items, shared_str key, Flags32* val, u32 mask, LPCSTR c0 = nullptr, LPCSTR c1 = nullptr, u32 flags = 0) = 0;
    virtual Token8Value* CreateToken8(PropItemVec& items, shared_str key, u8* val, const xr_token* token) = 0;
    virtual Token16Value* CreateToken16(PropItemVec& items, shared_str key, u16* val, xr_token* token) = 0;
    virtual Token32Value* CreateToken32(PropItemVec& items, shared_str key, u32* val, xr_token* token) = 0;
    virtual RToken8Value* CreateRToken8(
        PropItemVec& items, shared_str key, u8* val, xr_rtoken* token, u32 t_cnt) = 0;
    virtual RToken16Value* CreateRToken16(
        PropItemVec& items, shared_str key, u16* val, xr_rtoken* token, u32 t_cnt) = 0;
    virtual RToken32Value* CreateRToken32(
        PropItemVec& items, shared_str key, u32* val, xr_rtoken* token, u32 t_cnt) = 0;
    virtual RListValue* CreateRList(
        PropItemVec& items, shared_str key, shared_str* val, shared_str* lst, u32 cnt) = 0;
    virtual U32Value* CreateColor(PropItemVec& items, shared_str key, u32* val) = 0;
    virtual ColorValue* CreateFColor(PropItemVec& items, shared_str key, Fcolor* val) = 0;
    virtual VectorValue* CreateVColor(PropItemVec& items, shared_str key, Fvector* val) = 0;
    virtual RTextValue* CreateRText(PropItemVec& items, shared_str key, shared_str* val) = 0;
    virtual STextValue* CreateSText(PropItemVec& items, shared_str key, xr_string* val) = 0;
    virtual WaveValue* CreateWave(PropItemVec& items, shared_str key, WaveForm* val) = 0;
    virtual FloatValue* CreateTime(
        PropItemVec& items, shared_str key, float* val, float mn = 0.f, float mx = 86400.f) = 0;
    virtual ShortcutValue* CreateShortcut(PropItemVec& items, shared_str key, xr_shortcut* val) = 0;

    virtual FloatValue* CreateAngle(PropItemVec& items, shared_str key, float* val, float mn = flt_min,
        float mx = flt_max, float inc = 0.01f, int decim = 2) = 0;
    virtual VectorValue* CreateAngle3(PropItemVec& items, shared_str key, Fvector* val, float mn = flt_min,
        float mx = flt_max, float inc = 0.01f, int decim = 2) = 0;
    virtual RTextValue* CreateName(PropItemVec& items, shared_str key, shared_str* val, ListItem* owner) = 0;
    virtual RTextValue* CreateNameCB(PropItemVec& items, shared_str key, shared_str* val,
        TOnDrawTextEvent = nullptr, RTextValue::TOnBeforeEditEvent = nullptr, RTextValue::TOnAfterEditEvent = nullptr) = 0;

    virtual GameTypeValue* CreateGameType(PropItemVec& items, shared_str key, GameTypeChooser* val) = 0;
    // obsolette
    virtual CTextValue* CreateCText(PropItemVec& items, shared_str key, pstr val, u32 sz) = 0;
    virtual CListValue* CreateCList(
        PropItemVec& items, shared_str key, pstr val, u32 sz, xr_string* lst, u32 cnt) = 0;
    virtual CTextValue* CreateCName(
        PropItemVec& items, shared_str key, pstr val, u32 sz, ListItem* owner) = 0;
    virtual TokenValueSH* CreateTokenSH(
        PropItemVec& items, shared_str key, u32* val, const TokenValueSH::Item* lst, u32 cnt) = 0;
    virtual CTextValue* CreateTexture(PropItemVec& items, shared_str key, pstr val, u32 sz) = 0;
};
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
extern "C" XR_EPROPS_API IPropHelper& PHelper();
#else
extern IPropHelper& PHelper();
#endif
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
// List
//------------------------------------------------------------------------------
class XR_EPROPS_API IListHelper
{
public:
    virtual ListItem* FindItem(ListItemsVec& items, LPCSTR key) = 0;
    virtual bool NameAfterEdit(ListItem* sender, LPCSTR value, shared_str& edit_val) = 0;

public:
    virtual ListItem* CreateItem(
        ListItemsVec& items, LPCSTR key, int type, u32 item_flags = 0, void* object = nullptr) = 0;
};

#ifdef __BORLANDC__
extern "C" XR_EPROPS_API IListHelper& LHelper();
#endif

#endif
