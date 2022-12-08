//---------------------------------------------------------------------------
#ifndef PropertiesListHelperH
#define PropertiesListHelperH

// refs
class ListItem;

//---------------------------------------------------------------------------
class CPropHelper : public IPropHelper
{
    PropItem* CreateItem(PropItemVec& items, const shared_str& key, EPropType type, u32 item_flags = 0);
    PropValue* AppendValue(
        PropItemVec& items, const shared_str& key, PropValue* val, EPropType type, u32 item_flags = 0);

public:
    virtual PropItem* FindItem(PropItemVec& items, shared_str key, EPropType type = PROP_UNDEF);

public:
    //------------------------------------------------------------------------------
    // predefind event routines
    virtual bool FvectorRDOnAfterEdit(PropValue* sender, Fvector& edit_val);
    virtual void FvectorRDOnBeforeEdit(PropValue* sender, Fvector& edit_val);
    virtual void FvectorRDOnDraw(PropValue* sender, xr_string& draw_val);
    virtual bool floatRDOnAfterEdit(PropValue* sender, float& edit_val);
    virtual void floatRDOnBeforeEdit(PropValue* sender, float& edit_val);
    virtual void floatRDOnDraw(PropValue* sender, xr_string& draw_val);
    // R-name edit
    virtual void NameBeforeEdit(PropValue* sender, shared_str& edit_val);
    virtual bool NameAfterEdit(PropValue* sender, shared_str& edit_val);
    virtual void NameDraw(PropValue* sender, xr_string& draw_val);
    // C-name edit
    virtual void CNameBeforeEdit(PropValue* sender, xr_string& edit_val);
    virtual bool CNameAfterEdit(PropValue* sender, xr_string& edit_val);
    virtual void CNameDraw(PropValue* sender, xr_string& draw_val);

public:
    virtual CaptionValue* CreateCaption(PropItemVec& items, shared_str key, shared_str val);
    virtual CanvasValue* CreateCanvas(PropItemVec& items, shared_str key, shared_str val, int height);
    virtual ButtonValue* CreateButton(
        PropItemVec& items, shared_str key, shared_str val, u32 flags, ButtonValue::TOnBtnClick onclick = 0);
    virtual ChooseValue* CreateChoose(PropItemVec& items, shared_str key, shared_str* val, u32 mode,
        LPCSTR path = 0, void* fill_param = 0, u32 sub_item_count = 1, u32 choose_flags = cfAllowNone);
    virtual S8Value* CreateS8(
        PropItemVec& items, shared_str key, s8* val, s8 mn = 0, s8 mx = 100, s8 inc = 1);
    virtual S16Value* CreateS16(
        PropItemVec& items, shared_str key, s16* val, s16 mn = 0, s16 mx = 100, s16 inc = 1);
    virtual S32Value* CreateS32(
        PropItemVec& items, shared_str key, s32* val, s32 mn = 0, s32 mx = 100, s32 inc = 1);
    virtual U8Value* CreateU8(
        PropItemVec& items, shared_str key, u8* val, u8 mn = 0, u8 mx = 100, u8 inc = 1);
    virtual U16Value* CreateU16(
        PropItemVec& items, shared_str key, u16* val, u16 mn = 0, u16 mx = 100, u16 inc = 1);
    virtual U32Value* CreateU32(
        PropItemVec& items, shared_str key, u32* val, u32 mn = 0, u32 mx = 100, u32 inc = 1);
    virtual FloatValue* CreateFloat(PropItemVec& items, shared_str key, float* val, float mn = 0.f,
        float mx = 1.f, float inc = 0.01f, int decim = 2);
    virtual BOOLValue* CreateBOOL(PropItemVec& items, shared_str key, BOOL* val);
    virtual VectorValue* CreateVector(PropItemVec& items, shared_str key, Fvector* val, float mn = 0.f,
        float mx = 1.f, float inc = 0.01f, int decim = 2);
    virtual Flag8Value* CreateFlag8(
        PropItemVec& items, shared_str key, Flags8* val, u8 mask, LPCSTR c0 = 0, LPCSTR c1 = 0, u32 flags = 0);
    virtual Flag16Value* CreateFlag16(
        PropItemVec& items, shared_str key, Flags16* val, u16 mask, LPCSTR c0 = 0, LPCSTR c1 = 0, u32 flags = 0);
    virtual Flag32Value* CreateFlag32(
        PropItemVec& items, shared_str key, Flags32* val, u32 mask, LPCSTR c0 = 0, LPCSTR c1 = 0, u32 flags = 0);
    virtual Token8Value* CreateToken8(PropItemVec& items, shared_str key, u8* val, xr_token* token);
    virtual Token16Value* CreateToken16(PropItemVec& items, shared_str key, u16* val, xr_token* token);
    virtual Token32Value* CreateToken32(PropItemVec& items, shared_str key, u32* val, xr_token* token);
    virtual RToken8Value* CreateRToken8(
        PropItemVec& items, shared_str key, u8* val, xr_rtoken* token, u32 t_cnt);
    virtual RToken16Value* CreateRToken16(
        PropItemVec& items, shared_str key, u16* val, xr_rtoken* token, u32 t_cnt);
    virtual RToken32Value* CreateRToken32(
        PropItemVec& items, shared_str key, u32* val, xr_rtoken* token, u32 t_cnt);
    virtual RListValue* CreateRList(
        PropItemVec& items, shared_str key, shared_str* val, shared_str* lst, u32 cnt);
    virtual U32Value* CreateColor(PropItemVec& items, shared_str key, u32* val);
    virtual ColorValue* CreateFColor(PropItemVec& items, shared_str key, Fcolor* val);
    virtual VectorValue* CreateVColor(PropItemVec& items, shared_str key, Fvector* val);
    virtual RTextValue* CreateRText(PropItemVec& items, shared_str key, shared_str* val);
    virtual STextValue* CreateSText(PropItemVec& items, shared_str key, xr_string* val);
    virtual WaveValue* CreateWave(PropItemVec& items, shared_str key, WaveForm* val);
    virtual FloatValue* CreateTime(
        PropItemVec& items, shared_str key, float* val, float mn = 0.f, float mx = 86400.f);
    virtual ShortcutValue* CreateShortcut(PropItemVec& items, shared_str key, xr_shortcut* val);

    virtual FloatValue* CreateAngle(PropItemVec& items, shared_str key, float* val, float mn = flt_min,
        float mx = flt_max, float inc = 0.01f, int decim = 2);
    virtual VectorValue* CreateAngle3(PropItemVec& items, shared_str key, Fvector* val, float mn = flt_min,
        float mx = flt_max, float inc = 0.01f, int decim = 2);
    virtual RTextValue* CreateName(PropItemVec& items, shared_str key, shared_str* val, ListItem* owner);
    virtual RTextValue* CreateNameCB(PropItemVec& items, shared_str key, shared_str* val,
        TOnDrawTextEvent = 0, RTextValue::TOnBeforeEditEvent = 0, RTextValue::TOnAfterEditEvent = 0);

    virtual GameTypeValue* CreateGameType(PropItemVec& items, shared_str key, GameTypeChooser* val);

    // obsolette
    virtual CTextValue* CreateCText(PropItemVec& items, shared_str key, pstr val, u32 sz);
    virtual CListValue* CreateCList(
        PropItemVec& items, shared_str key, pstr val, u32 sz, xr_string* lst, u32 cnt);
    virtual CTextValue* CreateCName(PropItemVec& items, shared_str key, pstr val, u32 sz, ListItem* owner);
    virtual TokenValueSH* CreateTokenSH(
        PropItemVec& items, shared_str key, u32* val, const TokenValueSH::Item* lst, u32 cnt);
    virtual CTextValue* CreateTexture(PropItemVec& items, shared_str key, pstr val, u32 sz);
};
//---------------------------------------------------------------------------
#endif
