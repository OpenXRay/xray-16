//---------------------------------------------------------------------------
#ifndef ItemListTypesH
#define ItemListTypesH

//---------------------------------------------------------------------------

class ListItem
{
    friend class CListHelper;
    friend class TItemList;
    shared_str key;
    int type;
    void* item;

public:
    typedef fastdelegate::FastDelegate1<ListItem*> TOnListItemFocused;
    typedef fastdelegate::FastDelegate1<ListItem*> TOnClick;
    TOnClick OnClickEvent;
    TOnListItemFocused OnItemFocused;
    TOnDrawThumbnail OnDrawThumbnail;

public:
    int tag;
    LPVOID m_Object;
    int icon_index;
    u32 prop_color;

public:
    enum
    {
        flShowCB = (1 << 0),
        flCBChecked = (1 << 1),
        flDrawThumbnail = (1 << 2),
        flDrawCanvas = (1 << 3),
        flSorted = (1 << 4),
        flHidden = (1 << 5),
    };
    Flags32 m_Flags;

public:
    ListItem(int _type)
        : type(_type), prop_color(0), item(nullptr), key(nullptr), tag(0), icon_index(-1), OnDrawThumbnail(nullptr), OnItemFocused(nullptr),
          m_Object(nullptr)
    {
        m_Flags.zero();
    }
    virtual ~ListItem(){};
    void SetName(LPCSTR _key) { key = _key; }
    IC void Visible(BOOL val) { m_Flags.set(flHidden, !val); }
    IC BOOL Visible() const { return !m_Flags.test(flHidden); }
    IC int Type() { return type; }
    IC void* Item() { return item; }
    IC LPCSTR Key() { return key.c_str(); }
    IC void SetIcon(int index) { icon_index = index; }
};

using ListItemsVec = xr_vector<ListItem*>;
//---------------------------------------------------------------------------
#endif
