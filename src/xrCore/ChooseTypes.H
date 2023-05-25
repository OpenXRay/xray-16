#pragma once
#include "xrCore/fastdelegate.h"
#include "xrCommon/xr_vector.h"

// fwd. decl.
template <class T>
struct _rect;
using Irect = _rect<int>;


enum EChooseMode
{
    smCustom = 0,
    smSoundSource,
    smSoundEnv,
    smObject,
    smGroup,
    smEShader,
    smCShader,
    smPE,
    smParticles,
    smTexture,
    smTextureRaw,
    smEntityType,
    smSpawnItem,
    smLAnim,
    smVisual,
    smSkeletonAnims,
    smSkeletonBones,
    smSkeletonBonesInObject,
    smGameMaterial,
    smGameAnim,
    smGameSMotions,
};

enum EChooseFlags
{
    cfMultiSelect = (1 << 0),
    cfAllowNone = (1 << 1),
    cfFullExpand = (1 << 2),
};

struct SChooseItem
{
    SChooseItem() = default;
    shared_str name;
    shared_str hint;
    SChooseItem(LPCSTR nm, LPCSTR ht) : name(nm), hint(ht) {}
};

DEFINE_VECTOR(SChooseItem, ChooseItemVec, ChooseItemVecIt);
#ifndef ImTextureID
using ImTextureID = void *;
#endif
// typedef
typedef fastdelegate::FastDelegate2<ChooseItemVec &, void *> TOnChooseFillItems;
typedef fastdelegate::FastDelegate2<SChooseItem *, PropItemVec &> TOnChooseSelectItem;
typedef fastdelegate::FastDelegate2<LPCSTR, ImTextureID &> TGetTexture;
typedef fastdelegate::FastDelegate0<> TOnChooseClose;

typedef void (*TOnChooseFillEvents)();

struct SChooseEvents
{
    enum
    {
        flAnimated = (1 << 0),
        flClearTexture = (1 << 1),
    };
    shared_str caption;
    TOnChooseFillItems on_fill;
    TOnChooseSelectItem on_sel;
    TGetTexture on_get_texture;
    TOnChooseClose on_close;
    Flags32 flags;
    SChooseEvents()
    {
        caption = "Select Item";
        flags.zero();
    }
    SChooseEvents(LPCSTR capt, TOnChooseFillItems f, TOnChooseSelectItem s, TGetTexture t, TOnChooseClose c, u32 fl)
    {
        Set(capt, f, s, t, c, fl);
    }
    void Set(LPCSTR capt, TOnChooseFillItems f, TOnChooseSelectItem s, TGetTexture t, TOnChooseClose c, u32 fl)
    {
        caption = capt;
        on_fill = f;
        on_sel = s;
        on_get_texture = t;
        on_close = c;
        flags.assign(fl);
    }
};

#define NONE_CAPTION "<none>"
