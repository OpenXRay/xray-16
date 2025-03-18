////////////////////////////////////////////////////////////////////////////
//	Module 		: script_properties_list_helper.h
//	Created 	: 14.07.2004
//  Modified 	: 14.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script properties list helper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCDB/xrCDB.h"
#include "xrSound/Sound.h"

#include "xrServerEntities/xrEProps.h"
#include "xrServerEntities/script_rtoken_list.h"

class CScriptTokenList;

struct CScriptPropertiesListHelper
{
public:
    bool FvectorRDOnAfterEdit(PropValue* sender, Fvector& edit_val);
    void FvectorRDOnBeforeEdit(PropValue* sender, Fvector& edit_val);
    //			void 				FvectorRDOnDraw		(PropValue* sender,  pcstr& draw_val);
    bool floatRDOnAfterEdit(PropValue* sender, float& edit_val);
    void floatRDOnBeforeEdit(PropValue* sender, float& edit_val);
    //			void 				floatRDOnDraw		(PropValue* sender,  pcstr& draw_val);
    // name edit
    bool NameAfterEdit(PropValue* sender, pcstr& edit_val);
    void NameBeforeEdit(PropValue* sender, pcstr& edit_val);
    //			void 				NameDraw			(PropValue* sender,  pcstr& draw_val);
public:
    CaptionValue* CreateCaption(PropItemVec* items, pcstr key, pcstr val);
    CanvasValue* CreateCanvas(PropItemVec* items, pcstr key, pcstr val, int height);
    ButtonValue* CreateButton(PropItemVec* items, pcstr key, pcstr val, u32 flags);

    ChooseValue* CreateChoose(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mode);
    ChooseValue* CreateChoose(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mode, pcstr path);
    ChooseValue* CreateChoose(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mode, pcstr path, pcstr fill_param);
    ChooseValue* CreateChoose(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mode,
        pcstr path, pcstr fill_param, u32 sub_item_cnt);

    //			S8Value* 			CreateS8		    (PropItemVec* items, pcstr key, luabind::object object, pcstr
    // name);
    //			S8Value* 			CreateS8		    (PropItemVec* items, pcstr key, luabind::object object, pcstr name,
    //s8
    // mn);
    //			S8Value* 			CreateS8		    (PropItemVec* items, pcstr key, luabind::object object, pcstr name, s8
    //mn,
    // s8 mx);
    //			S8Value* 			CreateS8		    (PropItemVec* items, pcstr key, luabind::object object, pcstr name, s8
    //mn,
    // s8 mx, s8 inc);

    S16Value* CreateS16(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    S16Value* CreateS16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, s16 mn);
    S16Value* CreateS16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, s16 mn, s16 mx);
    S16Value* CreateS16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, s16 mn, s16 mx, s16 inc);

    S32Value* CreateS32(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    S32Value* CreateS32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, s32 mn);
    S32Value* CreateS32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, s32 mn, s32 mx);
    S32Value* CreateS32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, s32 mn, s32 mx, s32 inc);

    U8Value* CreateU8(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    U8Value* CreateU8(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mn);
    U8Value* CreateU8(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mn, u8 mx);
    U8Value* CreateU8(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mn, u8 mx, u8 inc);

    U16Value* CreateU16(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    U16Value* CreateU16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mn);
    U16Value* CreateU16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mn, u16 mx);
    U16Value* CreateU16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mn, u16 mx, u16 inc);

    U32Value* CreateU32(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    U32Value* CreateU32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mn);
    U32Value* CreateU32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mn, u32 mx);
    U32Value* CreateU32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mn, u32 mx, u32 inc);

    FloatValue* CreateFloat(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    FloatValue* CreateFloat(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn);
    FloatValue* CreateFloat(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx);
    FloatValue* CreateFloat(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc);
    FloatValue* CreateFloat(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc, int decim);

    BOOLValue* CreateBOOL(PropItemVec* items, pcstr key, luabind::object object, luabind::object table, pcstr name);

    VectorValue* CreateVector(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    VectorValue* CreateVector(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn);
    VectorValue* CreateVector(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx);
    VectorValue* CreateVector(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc);
    VectorValue* CreateVector(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc, int decim);

    Flag8Value* CreateFlag8(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mask);
    Flag8Value* CreateFlag8(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mask, pcstr c0);
    Flag8Value* CreateFlag8(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mask, pcstr c0, pcstr c1);
    Flag8Value* CreateFlag8(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u8 mask, pcstr c0, pcstr c1, u32 flags);

    Flag16Value* CreateFlag16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mask);
    Flag16Value* CreateFlag16(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mask, pcstr c0);
    Flag16Value* CreateFlag16(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mask, pcstr c0, pcstr c1);
    Flag16Value* CreateFlag16(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u16 mask, pcstr c0, pcstr c1, u32 flags);

    Flag32Value* CreateFlag32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mask);
    Flag32Value* CreateFlag32(PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mask, pcstr c0);
    Flag32Value* CreateFlag32(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mask, pcstr c0, pcstr c1);
    Flag32Value* CreateFlag32(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, u32 mask, pcstr c0, pcstr c1, u32 flags);

    Token8Value* CreateToken8(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, CScriptTokenList* token);
    Token16Value* CreateToken16(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, CScriptTokenList* token);
    Token32Value* CreateToken32(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, CScriptTokenList* token);
    //			RToken8Value* 		CreateRToken8	    (PropItemVec* items, pcstr key, luabind::object object, pcstr
    //name,
    // xr_vector<xr_rtoken>* token);
    //			RToken16Value* 		CreateRToken16	    (PropItemVec* items, pcstr key, luabind::object object, pcstr
    //name,
    // xr_vector<xr_rtoken>* token);
    //			RToken32Value* 		CreateRToken32	    (PropItemVec* items, pcstr key, luabind::object object, pcstr
    //name,
    // xr_vector<xr_rtoken>* token);
    //			TokenValueSH*   	CreateTokenSH	    (PropItemVec* items, pcstr key, luabind::object object, pcstr
    //name,
    // u32 cnt, const TokenValueSH::Item* lst);
    RListValue* CreateRList(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, CScriptRTokenList* tokens);
    U32Value* CreateColor(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    ColorValue* CreateFColor(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    VectorValue* CreateVColor(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    RTextValue* CreateRText(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    //			WaveValue* 			CreateWave		    (PropItemVec* items, pcstr key, WaveForm* val);
    FloatValue* CreateTime(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    FloatValue* CreateTime(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn);
    FloatValue* CreateTime(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx);

    FloatValue* CreateAngle(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    FloatValue* CreateAngle(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn);
    FloatValue* CreateAngle(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx);
    FloatValue* CreateAngle(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc);
    FloatValue* CreateAngle(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc, int decim);

    VectorValue* CreateAngle3(PropItemVec* items, pcstr key, luabind::object object, pcstr name);
    VectorValue* CreateAngle3(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn);
    VectorValue* CreateAngle3(PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx);
    VectorValue* CreateAngle3(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc);
    VectorValue* CreateAngle3(
        PropItemVec* items, pcstr key, luabind::object object, pcstr name, float mn, float mx, float inc, int decim);
    //			RTextValue* 		CreateName		    (PropItemVec* items, pcstr key, luabind::object object, pcstr
    //name,
    // ListItem* owner);
    //			RTextValue* 		CreateNameCB		(PropItemVec* items, pcstr key, luabind::object object, pcstr
    //name,
    // TOnDrawTextEvent=0, RTextValue::TOnBeforeEditEvent=0, RTextValue::TOnAfterEditEvent=0
};
