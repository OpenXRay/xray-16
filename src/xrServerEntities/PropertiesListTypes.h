//---------------------------------------------------------------------------
#pragma once

#include "xrEngine/WaveForm.h"
#include "gametype_chooser.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/_rect.h"
#include "xrCore/xr_trims.h"
#include "xrCore/xr_shortcut.h"
#include "xrCore/xr_token.h"
//#include "xrCore/xrCore.h"

#ifdef __BORLANDC__
#include "ElTree.hpp"
#endif

// fwd. decl.
using RStringVec = xr_vector<shared_str>;
struct xr_rtoken;

#pragma pack(push, 1)

//---------------------------------------------------------------------------
enum EPropType
{
    PROP_UNDEF = -1,
    PROP_CAPTION = 0x1000,
    PROP_SHORTCUT,
    PROP_BUTTON,
    PROP_CHOOSE,
    PROP_NUMERIC, // {u8,u16,u32,s8,s16,s32,f32}
    PROP_BOOLEAN,
    PROP_FLAG,
    PROP_VECTOR,
    PROP_TOKEN,
    PROP_RTOKEN,
    PROP_RLIST,
    PROP_COLOR,
    PROP_FCOLOR,
    PROP_VCOLOR,
    PROP_RTEXT,
    PROP_STEXT,
    PROP_WAVE,
    PROP_CANVAS,
    PROP_TIME,

    PROP_CTEXT,
    PROP_CLIST,
    PROP_SH_TOKEN,
    PROP_TEXTURE2,
    PROP_GAMETYPE,
};

// refs
struct xr_token;
class PropValue;
class PropItem;
using PropItemVec = xr_vector<PropItem*>;

//------------------------------------------------------------------------------
#include "xrCore/ChooseTypes.H"
//------------------------------------------------------------------------------
typedef fastdelegate::FastDelegate2<PropValue*, xr_string&> TOnDrawTextEvent;
typedef fastdelegate::FastDelegate1<PropItem*> TOnClick;
//------------------------------------------------------------------------------

class PropValue
{
    friend class CPropHelper;
    friend class PropItem;

protected:
    PropItem* m_Owner;

public:
    u32 tag;

    // base events
    typedef fastdelegate::FastDelegate1<PropValue*> TOnChange;

    TOnChange OnChangeEvent;

    PropValue() : m_Owner(nullptr), tag(0), OnChangeEvent(nullptr) {}
    virtual ~PropValue() {}
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText) = 0;
    virtual void ResetValue() = 0;
    virtual bool Equal(PropValue* prop) = 0;
    PropItem* Owner() { return m_Owner; }
};
//------------------------------------------------------------------------------

template <class T>
void set_value(T& val, const T& _val)
{
    val = _val;
}

template <class T>
class CustomValue : public PropValue
{
public:
    typedef T TYPE;

    TYPE init_value;
    TYPE* value;

    typedef fastdelegate::FastDelegate2<PropValue*, T&> TOnBeforeEditEvent;
    typedef fastdelegate::FastDelegate2<PropValue*, T&, bool> TOnAfterEditEvent;
    TOnBeforeEditEvent OnBeforeEditEvent;
    TOnAfterEditEvent OnAfterEditEvent;

    CustomValue(T* val)
    {
        OnBeforeEditEvent = 0;
        OnAfterEditEvent = 0;
        set_value(value, val);
        set_value(init_value, *val);
    }

    virtual xr_string GetDrawText(TOnDrawTextEvent /*OnDrawText*/) { return ""; }

    virtual bool Equal(PropValue* val)
    {
        CustomValue<T>* prop = (CustomValue<T>*)val;
        return (*value == *prop->value);
    }

    virtual const T& GetValue() { return *value; }

    virtual void ResetValue() { set_value(*value, init_value); }

    bool ApplyValue(const T& val)
    {
        if (!(*value == val))
        {
            set_value(*value, val);
            return true;
        }
        return false;
    }
};

class PropItem
{
    friend class CPropHelper;
    friend class TProperties;
    shared_str key;
    EPropType type;
    void* item;

public:
    using PropValueVec = xr_vector<PropValue*>;

private:
    PropValueVec values;
#ifdef WINDOWS
    TProperties* m_Owner;
#endif
    // events
public:
    typedef fastdelegate::FastDelegate1<PropItem*> TOnPropItemFocused;
    TOnDrawTextEvent OnDrawTextEvent;
    TOnPropItemFocused OnItemFocused;
    TOnClick OnClickEvent;

public:
    u32 prop_color;
    u32 val_color;
    Irect draw_rect;

public:
    enum
    {
        flDisabled = (1 << 0),
        flShowCB = (1 << 1),
        flCBChecked = (1 << 2),
        flMixed = (1 << 3),
        flDrawThumbnail = (1 << 4),
        flSorted = (1 << 5),
    };
    Flags32 m_Flags;

public:
    PropItem(EPropType _type)
        : type(_type), prop_color(0), val_color(0), item(nullptr), key(nullptr), OnClickEvent(nullptr), OnDrawTextEvent(nullptr),
          OnItemFocused(nullptr)
    {
        m_Flags.zero();
    }
    virtual ~PropItem()
    {
        for (auto it = values.begin(); values.end() != it; ++it)
            xr_delete(*it);
    }
#ifdef WINDOWS
    TProperties* Owner() { return m_Owner; }
#endif
    void SetName(const shared_str& name) { key = name; }

    void ResetValues()
    {
        for (auto it = values.begin(); values.end() != it; ++it)
            (*it)->ResetValue();
        CheckMixed();
    }

    void AppendValue(PropValue* value)
    {
        if (!values.empty() && !value->Equal(values.front()))
            m_Flags.set(flMixed, true);
        values.push_back(value);
    }

    xr_string GetDrawText()
    {
        VERIFY(!values.empty());
        return m_Flags.is(flMixed) ? xr_string("(mixed)") : values.front()->GetDrawText(OnDrawTextEvent);
    }

    void CheckMixed()
    {
        m_Flags.set(flMixed, false);
        if (values.size() > 1)
        {
            auto F = values.begin();
            auto it = F;
            ++it;
            for (; values.end() != it; ++it)
            {
                if (!(*it)->Equal(*F))
                {
                    m_Flags.set(flMixed, true);
                    break;
                }
            }
        }
    }

    template <class T1, class T2>
    void BeforeEdit(T2& val)
    {
        T1* CV = smart_cast<T1*>(values.front());
        VERIFY(CV);
        if (!CV->OnBeforeEditEvent.empty())
            CV->OnBeforeEditEvent(CV, val);
    }
    template <class T1, class T2>
    bool AfterEdit(T2& val)
    {
        T1* CV = smart_cast<T1*>(values.front());
        VERIFY(CV);
        if (!CV->OnAfterEditEvent.empty())
            return CV->OnAfterEditEvent(CV, val);
        return true;
    }
    template <class T1, class T2>
    bool ApplyValue(const T2& val)
    {
        bool bChanged = false;
        m_Flags.set(flMixed, false);
        for (auto it = values.begin(); values.end() != it; ++it)
        {
            T1* CV = smart_cast<T1*>(*it);
            VERIFY(CV);
            if (CV->ApplyValue(val))
            {
                bChanged = true;
                if (!CV->OnChangeEvent.empty())
                    CV->OnChangeEvent(*it);
            }
            if (!CV->Equal(values.front()))
                m_Flags.set(flMixed, true);
        }
        return bChanged;
    }

    PropValueVec& Values() { return values; }

    PropValue* GetFrontValue()
    {
        VERIFY(!values.empty());
        return values.front();
    };
    EPropType Type() { return type; }
#ifdef __BORLANDC__
    IC TElTreeItem* Item() { return (TElTreeItem*)item; }
#endif
    LPCSTR Key() { return key.c_str(); }
    void Enable(BOOL val) { m_Flags.set(flDisabled, !val); }
    BOOL Enabled() { return !m_Flags.is(flDisabled); }

    void OnChange()
    {
        for (auto it = values.begin(); values.end() != it; ++it)
            if (!(*it)->OnChangeEvent.empty())
                (*it)->OnChangeEvent(*it);
    }
    /*
        template <class T1, class T2>
        IC void				OnBeforeEdit	()
        {
            for (auto it=values.begin(); values.end() != it; ++it){
                T1* CV		= smart_cast<T1*>(*it); VERIFY(CV);
                if (CV->OnChangeEvent) 		CV->OnChangeEvent(*it);
            }
        }
    */
};

//------------------------------------------------------------------------------
// values
//------------------------------------------------------------------------------
class CaptionValue : public PropValue
{
    shared_str value;

public:
    CaptionValue(const shared_str& val) { value = val; }
    virtual xr_string GetDrawText(TOnDrawTextEvent) { return value.c_str() ? value.c_str() : ""; }
    virtual void ResetValue() {}
    virtual bool Equal(PropValue* val) { return (value == ((CaptionValue*)val)->value); }
    bool ApplyValue(const shared_str& val)
    {
        value = val;
        return false;
    }
};

class CanvasValue : public PropValue
{
    shared_str value;

public:
    typedef fastdelegate::FastDelegate3<CanvasValue*, CanvasValue*, bool&> TOnTestEqual;
    typedef fastdelegate::FastDelegate3<CanvasValue*, void* /* TCanvas* */, const Irect&> TOnDrawCanvasEvent;

    int height;
    TOnTestEqual OnTestEqual;
    TOnDrawCanvasEvent OnDrawCanvasEvent;

    CanvasValue(const shared_str& val, int h) : OnDrawCanvasEvent(nullptr), OnTestEqual(nullptr), height(h) { value = val; }
    virtual xr_string GetDrawText(TOnDrawTextEvent) { return value.c_str() ? value.c_str() : ""; }
    virtual void ResetValue() {}
    virtual bool Equal(PropValue* val)
    {
        if (!OnTestEqual.empty())
        {
            bool res = true;
            OnTestEqual(this, (CanvasValue*)val, res);
            return res;
        }
        return false;
    }
};

class ButtonValue : public PropValue
{
public:
    RStringVec value;
    int btn_num;
    typedef fastdelegate::FastDelegate3<ButtonValue*, bool&, bool&> TOnBtnClick;
    TOnBtnClick OnBtnClickEvent;
    enum
    {
        flFirstOnly = (1 << 0)
    };
    Flags32 m_Flags;

    ButtonValue(const shared_str& val, u32 flags)
    {
        m_Flags.assign(flags);
        OnBtnClickEvent = nullptr;
        btn_num = -1;
        xr_string v;
        int cnt = _GetItemCount(val.c_str());
        for (int k = 0; k < cnt; ++k)
            value.push_back(_GetItem(val.c_str(), k, v));
    }
    virtual xr_string GetDrawText(TOnDrawTextEvent)
    {
        shared_str t = _ListToSequence(value);
        return t.c_str() ? t.c_str() : "";
    }
    virtual void ResetValue() {}
    virtual bool Equal(PropValue* /*val*/) { return true; }
    bool OnBtnClick(bool& bSafe)
    {
        if (!OnBtnClickEvent.empty())
        {
            bool bDModif = true;
            OnBtnClickEvent(this, bDModif, bSafe);
            return bDModif;
        }
        else
            return false;
    }
};
//------------------------------------------------------------------------------

IC bool operator==(const xr_shortcut& A, const xr_shortcut& B) { return !!A.similar(B); }
class ShortcutValue : public CustomValue<xr_shortcut>
{
public:
    typedef fastdelegate::FastDelegate3<ShortcutValue*, const xr_shortcut&, bool&> TOnValidateResult;
    TOnValidateResult OnValidateResultEvent;

    ShortcutValue(TYPE* val) : CustomValue<xr_shortcut>(val) {}
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText);
    bool ApplyValue(const xr_shortcut& val)
    {
        if (!(*value == val))
        {
            bool allow = true;
            if (!OnValidateResultEvent.empty())
                OnValidateResultEvent(this, val, allow);
            if (allow)
            {
                set_value(*value, val);
                return true;
            }
        }
        return false;
    }
};

class RTextValue : public CustomValue<shared_str>
{
public:
    RTextValue(TYPE* val) : CustomValue<shared_str>(val){};
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string txt = GetValue().c_str() ? GetValue().c_str() : "";
        if (!OnDrawText.empty())
            OnDrawText(this, txt);
        return txt;
    }
};

class STextValue : public CustomValue<xr_string>
{
public:
    STextValue(TYPE* val) : CustomValue<xr_string>(val){};
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string txt = GetValue();
        if (!OnDrawText.empty())
            OnDrawText(this, txt);
        return txt;
    }
};

class CTextValue : public PropValue
{
    xr_string init_value;

public:
    LPSTR value;

    typedef fastdelegate::FastDelegate2<PropValue*, xr_string&> TOnBeforeEditEvent;
    typedef fastdelegate::FastDelegate2<PropValue*, xr_string&, bool> TOnAfterEditEvent;

    TOnBeforeEditEvent OnBeforeEditEvent;
    TOnAfterEditEvent OnAfterEditEvent;

    int lim;

    CTextValue(LPSTR val, int _lim) : value(val), init_value(val), lim(_lim)
    {
        OnBeforeEditEvent = nullptr;
        OnAfterEditEvent = nullptr;
    };
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string txt = GetValue();
        if (!OnDrawText.empty())
            OnDrawText(this, txt);
        return txt;
    }

    virtual bool Equal(PropValue* val) { return (0 == xr_strcmp(value, ((CTextValue*)val)->value)); }

    bool ApplyValue(LPCSTR val)
    {
        if (0 != xr_strcmp(value, val))
        {
            xr_strcpy(value, xr_strlen(val) + 1, val);
            return true;
        }
        return false;
    }

    LPSTR GetValue() { return value; }
    virtual void ResetValue() { xr_strcpy(value, init_value.size() + 1, init_value.c_str()); }
};
//------------------------------------------------------------------------------

class ChooseValue : public RTextValue
{
public:
    int subitem;
    u32 m_ChooseID;
    u32 m_ChooseFlags;
    shared_str m_StartPath;
    ChooseItemVec* m_Items;
    typedef fastdelegate::FastDelegate1<ChooseValue*> TOnChooseValueFill;
    TOnChooseValueFill OnChooseFillEvent;
    TOnDrawThumbnail OnDrawThumbnailEvent;
    void* m_FillParam;
    // utils
    void AppendChooseItem(LPCSTR name, LPCSTR hint)
    {
        VERIFY(m_Items);
        m_Items->push_back(SChooseItem(name, hint));
    }

    ChooseValue(shared_str* val, u32 cid, LPCSTR path, void* param, u32 sub_item_count, u32 choose_flags)
        : RTextValue(val), m_ChooseID(cid), m_StartPath(path), subitem(sub_item_count), m_Items(nullptr), m_FillParam(param),
          OnChooseFillEvent(nullptr), OnDrawThumbnailEvent(nullptr), m_ChooseFlags(choose_flags)
    {}
};

typedef CustomValue<BOOL> BOOLValue;
//------------------------------------------------------------------------------

IC bool operator==(const WaveForm& A, const WaveForm& B) { return !!A.Similar(B); }
class WaveValue : public CustomValue<WaveForm>
{
public:
    WaveValue(TYPE* val) : CustomValue<WaveForm>(val){};
    virtual xr_string GetDrawText(TOnDrawTextEvent) { return "[Wave]"; }
};

IC bool operator==(const GameTypeChooser& A, const GameTypeChooser& B)
{
    return A.m_GameType.flags == B.m_GameType.flags;
}
class GameTypeValue : public CustomValue<GameTypeChooser>
{
public:
    GameTypeValue(TYPE* val) : CustomValue<GameTypeChooser>(val){};
    virtual xr_string GetDrawText(TOnDrawTextEvent);
};

//------------------------------------------------------------------------------

IC bool operator==(const Fcolor& A, const Fcolor& B) { return !!A.similar_rgba(B); }
typedef CustomValue<Fcolor> ColorValue;
//------------------------------------------------------------------------------

template <class T>
class NumericValue : public CustomValue<T>
{
public:
    T lim_mn;
    T lim_mx;
    T inc;
    int dec;

public:
    NumericValue(T* val) : CustomValue<T>(val)
    {
        this->value = val;
        this->init_value = *this->value;
        dec = 0;
    };
    NumericValue(T* val, T mn, T mx, T increm, int decim)
        : CustomValue<T>(val), lim_mn(mn), lim_mx(mx), inc(increm), dec(decim)
    {
        clamp(*val, lim_mn, lim_mx);
        this->value = val;
        this->init_value = *this->value;
    };
    bool ApplyValue(const T& _val)
    {
        T val = _val;
        clamp(val, lim_mn, lim_mx);
        return CustomValue<T>::ApplyValue(val);
    }
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string draw_val;
        if (!OnDrawText.empty())
            OnDrawText(this, draw_val);
        else
            draw_sprintf(draw_val, *this->value, dec);
        return draw_val;
    }
};

//------------------------------------------------------------------------------
template <class T>
xr_string draw_sprintf(xr_string& s, const T& V, int tag)
{
    string256 tmp;
    xr_sprintf(tmp, sizeof(tmp), "%d", V);
    s = tmp;
    return s;
}
//------------------------------------------------------------------------------
IC xr_string draw_sprintf(xr_string& s, const float& V, int dec)
{
    string32 fmt;
    xr_sprintf(fmt, sizeof(fmt), "%%.%df", dec);
    string256 tmp;
    xr_sprintf(tmp, sizeof(tmp), fmt, V);
    s = tmp;
    return s;
}
//------------------------------------------------------------------------------
IC bool operator==(const Fvector& A, const Fvector& B) { return !!A.similar(B); }
IC void clamp(Fvector& V, const Fvector& mn, const Fvector& mx)
{
    clamp(V.x, mn.x, mx.x);
    clamp(V.y, mn.y, mx.y);
    clamp(V.z, mn.z, mx.z);
}
IC xr_string draw_sprintf(xr_string& s, const Fvector& V, int dec)
{
    string128 fmt;
    xr_sprintf(fmt, sizeof(fmt), "{%%.%df, %%.%df, %%.%df}", dec, dec, dec);
    string256 tmp;
    xr_sprintf(tmp, sizeof(tmp), fmt, V.x, V.y, V.z);
    s = tmp;
    return s;
}
//------------------------------------------------------------------------------
typedef NumericValue<u8> U8Value;
typedef NumericValue<u16> U16Value;
typedef NumericValue<u32> U32Value;
typedef NumericValue<s8> S8Value;
typedef NumericValue<s16> S16Value;
typedef NumericValue<s32> S32Value;
typedef NumericValue<float> FloatValue;
class VectorValue : public NumericValue<Fvector>
{
public:
    VectorValue(Fvector* val, float mn, float mx, float increment, int decimal) : NumericValue<Fvector>(val)
    {
        lim_mn.set(mn, mn, mn);
        lim_mx.set(mx, mx, mx);
        inc.set(increment, increment, increment);
        dec = decimal;
    };
};
//------------------------------------------------------------------------------

class FlagValueCustom
{
public:
    shared_str caption[2];
    enum
    {
        flInvertedDraw = (1 << 0),
    };
    Flags32 m_Flags;

    FlagValueCustom(u32 mask, LPCSTR c0, LPCSTR c1)
    {
        caption[0] = c0;
        caption[1] = c1;
        m_Flags.assign(mask);
    }
    virtual bool HaveCaption() { return caption[0].size() && caption[1].size(); }
    virtual bool GetValueEx() = 0;
};

template <class T>
class FlagValue : public CustomValue<T>, public FlagValueCustom
{
public:
    typedef T TYPE;
    typedef typename T::TYPE FLAG_TYPE;

    FLAG_TYPE mask;

    FlagValue(T* val, FLAG_TYPE _mask, LPCSTR c0, LPCSTR c1, u32 flags)
        : CustomValue<T>(val), FlagValueCustom(flags, c0, c1), mask(_mask)
    {}

    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string draw_val;
        if (!OnDrawText.empty())
            OnDrawText(this, draw_val);
        else
            return HaveCaption() ? caption[GetValueEx() ? 1 : 0].c_str() : "";
        return draw_val;
    }
    virtual bool Equal(PropValue* val) { return !!this->value->equal(*((FlagValue<T>*)val)->value, mask); }
    virtual const T& GetValue() { return *this->value; }
    virtual void ResetValue() { this->value->set(mask,  this->init_value.is(mask)); }
    virtual bool GetValueEx() { return !!this->value->is(mask); }

    bool ApplyValue(const T& val)
    {
        if (!val.equal(*this->value, mask))
        {
            this->value->set(mask, val.is(mask));
            return true;
        }
        return false;
    }
};
//------------------------------------------------------------------------------
typedef FlagValue<Flags8> Flag8Value;
typedef FlagValue<Flags16> Flag16Value;
typedef FlagValue<Flags32> Flag32Value;
//------------------------------------------------------------------------------
template <class T>
bool operator==(_flags<T> const& A, _flags<T> const& B)
{
    return A.flags == B.flags;
}
//------------------------------------------------------------------------------

class TokenValueCustom
{
public:
    xr_token* token;
    TokenValueCustom(xr_token* _token) : token(_token) {}
};

template <class T>
class TokenValue : public CustomValue<T>, public TokenValueCustom
{
public:
    TokenValue(T* val, xr_token* _token) : TokenValueCustom(_token), CustomValue<T>(val){}
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string draw_val;
        if (!OnDrawText.empty())
            OnDrawText(this, draw_val);
        else
            for (int i = 0; this->token[i].name; i++)
                if (this->token[i].id == (int)this->GetValue())
                    return this->token[i].name;
        return draw_val;
    }
};

//------------------------------------------------------------------------------
typedef TokenValue<u8> Token8Value;
typedef TokenValue<u16> Token16Value;
typedef TokenValue<u32> Token32Value;
//------------------------------------------------------------------------------

class RTokenValueCustom
{
public:
    xr_rtoken* token;
    u32 token_count;
    RTokenValueCustom(xr_rtoken* _token, u32 _t_cnt) : token(_token), token_count(_t_cnt) {}
};

template <class T>
class RTokenValue : public CustomValue<T>, public RTokenValueCustom
{
public:
    RTokenValue(T* val, xr_rtoken* _token, u32 _t_cnt) : CustomValue<T>(val), RTokenValueCustom(_token, _t_cnt){};
    virtual xr_string GetDrawText(TOnDrawTextEvent OnDrawText)
    {
        xr_string draw_val;
        if (!OnDrawText.empty())
            OnDrawText(this, draw_val);
        else
            for (u32 k = 0; k < token_count; k++)
                if ((T)token[k].id == this->GetValue())
                    return *token[k].name;
        return draw_val;
    }
};

//------------------------------------------------------------------------------
typedef RTokenValue<u8> RToken8Value;
typedef RTokenValue<u16> RToken16Value;
typedef RTokenValue<u32> RToken32Value;
//------------------------------------------------------------------------------

class TokenValueSH : public CustomValue<u32>
{
public:
    struct Item
    {
        u32 ID;
        string64 str;
    };
    u32 cnt;
    const Item* items;

    TokenValueSH(u32* val, const Item* _items, u32 _cnt) : CustomValue<u32>(val), cnt(_cnt), items(_items){};
    virtual xr_string GetDrawText(TOnDrawTextEvent /*OnDrawText*/)
    {
        u32 draw_val = GetValue();
        for (u32 i = 0; i < cnt; i++)
            if (items[i].ID == draw_val)
                return items[i].str;
        return nullptr;
    }
};
//------------------------------------------------------------------------------

class RListValue : public RTextValue
{
public:
    shared_str* items;
    u32 item_count;

    RListValue(shared_str* val, shared_str* _items, u32 cnt) : RTextValue(val), items(_items), item_count(cnt){};
    virtual bool Equal(PropValue* val)
    {
        if (items != ((RListValue*)val)->items)
        {
            m_Owner->m_Flags.set(PropItem::flDisabled, true);
            return false;
        }
        return RTextValue::Equal(val);
    }
};
class CListValue : public CTextValue
{
public:
    xr_string* items;
    u32 item_count;

    CListValue(LPSTR val, u32 sz, xr_string* _items, u32 cnt) : CTextValue(val, sz), items(_items), item_count(cnt){};
    virtual bool Equal(PropValue* val)
    {
        if (items != ((CListValue*)val)->items)
        {
            m_Owner->m_Flags.set(PropItem::flDisabled, true);
            return false;
        }
        return CTextValue::Equal(val);
    }
};
//------------------------------------------------------------------------------
#pragma pack(pop)
