////////////////////////////////////////////////////////////////////////////
//	Module 		: script_properties_list_helper.h
//	Created 	: 14.07.2004
//  Modified 	: 14.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script properties list helper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCDB/xrCDB.h"
#ifndef XRGAME_EXPORTS
#include "xrSound/Sound.h"
#endif
#include "xrServerEntities\xrEProps.h"
#include "xrServerEntities\script_rtoken_list.h"
#include "script_space.h"

class CScriptTokenList;

struct CScriptPropertiesListHelper
{
public:
    bool FvectorRDOnAfterEdit(PropValue* sender, Fvector& edit_val);
    void FvectorRDOnBeforeEdit(PropValue* sender, Fvector& edit_val);
    //			void 				FvectorRDOnDraw		(PropValue* sender,  const char*& draw_val);
    bool floatRDOnAfterEdit(PropValue* sender, float& edit_val);
    void floatRDOnBeforeEdit(PropValue* sender, float& edit_val);
    //			void 				floatRDOnDraw		(PropValue* sender,  const char*& draw_val);
    // name edit
    bool NameAfterEdit(PropValue* sender, const char*& edit_val);
    void NameBeforeEdit(PropValue* sender, const char*& edit_val);
    //			void 				NameDraw			(PropValue* sender,  const char*& draw_val);
public:
    CaptionValue* CreateCaption(PropItemVec* items, const char* key, const char* val);
    CanvasValue* CreateCanvas(PropItemVec* items, const char* key, const char* val, int height);
    ButtonValue* CreateButton(PropItemVec* items, const char* key, const char* val, u32 flags);

    ChooseValue* CreateChoose(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mode);
    ChooseValue* CreateChoose(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mode, const char* path);
    ChooseValue* CreateChoose(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mode, const char* path, const char* fill_param);
    ChooseValue* CreateChoose(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mode,
        const char* path, const char* fill_param, u32 sub_item_cnt);

    //			S8Value* 			CreateS8		    (PropItemVec* items, const char* key, luabind::object object, const char*
    // name);
    //			S8Value* 			CreateS8		    (PropItemVec* items, const char* key, luabind::object object, const char* name,
    //s8
    // mn);
    //			S8Value* 			CreateS8		    (PropItemVec* items, const char* key, luabind::object object, const char* name, s8
    //mn,
    // s8 mx);
    //			S8Value* 			CreateS8		    (PropItemVec* items, const char* key, luabind::object object, const char* name, s8
    //mn,
    // s8 mx, s8 inc);

    S16Value* CreateS16(PropItemVec* items, const char* key, luabind::object object, const char* name);
    S16Value* CreateS16(PropItemVec* items, const char* key, luabind::object object, const char* name, s16 mn);
    S16Value* CreateS16(PropItemVec* items, const char* key, luabind::object object, const char* name, s16 mn, s16 mx);
    S16Value* CreateS16(PropItemVec* items, const char* key, luabind::object object, const char* name, s16 mn, s16 mx, s16 inc);

    S32Value* CreateS32(PropItemVec* items, const char* key, luabind::object object, const char* name);
    S32Value* CreateS32(PropItemVec* items, const char* key, luabind::object object, const char* name, s32 mn);
    S32Value* CreateS32(PropItemVec* items, const char* key, luabind::object object, const char* name, s32 mn, s32 mx);
    S32Value* CreateS32(PropItemVec* items, const char* key, luabind::object object, const char* name, s32 mn, s32 mx, s32 inc);

    U8Value* CreateU8(PropItemVec* items, const char* key, luabind::object object, const char* name);
    U8Value* CreateU8(PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mn);
    U8Value* CreateU8(PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mn, u8 mx);
    U8Value* CreateU8(PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mn, u8 mx, u8 inc);

    U16Value* CreateU16(PropItemVec* items, const char* key, luabind::object object, const char* name);
    U16Value* CreateU16(PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mn);
    U16Value* CreateU16(PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mn, u16 mx);
    U16Value* CreateU16(PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mn, u16 mx, u16 inc);

    U32Value* CreateU32(PropItemVec* items, const char* key, luabind::object object, const char* name);
    U32Value* CreateU32(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mn);
    U32Value* CreateU32(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mn, u32 mx);
    U32Value* CreateU32(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mn, u32 mx, u32 inc);

    FloatValue* CreateFloat(PropItemVec* items, const char* key, luabind::object object, const char* name);
    FloatValue* CreateFloat(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn);
    FloatValue* CreateFloat(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx);
    FloatValue* CreateFloat(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc);
    FloatValue* CreateFloat(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc, int decim);

    BOOLValue* CreateBOOL(PropItemVec* items, const char* key, luabind::object object, luabind::object table, const char* name);

    VectorValue* CreateVector(PropItemVec* items, const char* key, luabind::object object, const char* name);
    VectorValue* CreateVector(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn);
    VectorValue* CreateVector(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx);
    VectorValue* CreateVector(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc);
    VectorValue* CreateVector(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc, int decim);

    Flag8Value* CreateFlag8(PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mask);
    Flag8Value* CreateFlag8(PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mask, const char* c0);
    Flag8Value* CreateFlag8(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mask, const char* c0, const char* c1);
    Flag8Value* CreateFlag8(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u8 mask, const char* c0, const char* c1, u32 flags);

    Flag16Value* CreateFlag16(PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mask);
    Flag16Value* CreateFlag16(PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mask, const char* c0);
    Flag16Value* CreateFlag16(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mask, const char* c0, const char* c1);
    Flag16Value* CreateFlag16(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u16 mask, const char* c0, const char* c1, u32 flags);

    Flag32Value* CreateFlag32(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mask);
    Flag32Value* CreateFlag32(PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mask, const char* c0);
    Flag32Value* CreateFlag32(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mask, const char* c0, const char* c1);
    Flag32Value* CreateFlag32(
        PropItemVec* items, const char* key, luabind::object object, const char* name, u32 mask, const char* c0, const char* c1, u32 flags);

    Token8Value* CreateToken8(
        PropItemVec* items, const char* key, luabind::object object, const char* name, CScriptTokenList* token);
    Token16Value* CreateToken16(
        PropItemVec* items, const char* key, luabind::object object, const char* name, CScriptTokenList* token);
    Token32Value* CreateToken32(
        PropItemVec* items, const char* key, luabind::object object, const char* name, CScriptTokenList* token);
    //			RToken8Value* 		CreateRToken8	    (PropItemVec* items, const char* key, luabind::object object, const char*
    //name,
    // RTokenVec* token);
    //			RToken16Value* 		CreateRToken16	    (PropItemVec* items, const char* key, luabind::object object, const char*
    //name,
    // RTokenVec* token);
    //			RToken32Value* 		CreateRToken32	    (PropItemVec* items, const char* key, luabind::object object, const char*
    //name,
    // RTokenVec* token);
    //			TokenValueSH*   	CreateTokenSH	    (PropItemVec* items, const char* key, luabind::object object, const char*
    //name,
    // u32 cnt, const TokenValueSH::Item* lst);
    RListValue* CreateRList(
        PropItemVec* items, const char* key, luabind::object object, const char* name, CScriptRTokenList* tokens);
    U32Value* CreateColor(PropItemVec* items, const char* key, luabind::object object, const char* name);
    ColorValue* CreateFColor(PropItemVec* items, const char* key, luabind::object object, const char* name);
    VectorValue* CreateVColor(PropItemVec* items, const char* key, luabind::object object, const char* name);
    RTextValue* CreateRText(PropItemVec* items, const char* key, luabind::object object, const char* name);
    //			WaveValue* 			CreateWave		    (PropItemVec* items, const char* key, WaveForm* val);
    FloatValue* CreateTime(PropItemVec* items, const char* key, luabind::object object, const char* name);
    FloatValue* CreateTime(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn);
    FloatValue* CreateTime(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx);

    FloatValue* CreateAngle(PropItemVec* items, const char* key, luabind::object object, const char* name);
    FloatValue* CreateAngle(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn);
    FloatValue* CreateAngle(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx);
    FloatValue* CreateAngle(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc);
    FloatValue* CreateAngle(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc, int decim);

    VectorValue* CreateAngle3(PropItemVec* items, const char* key, luabind::object object, const char* name);
    VectorValue* CreateAngle3(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn);
    VectorValue* CreateAngle3(PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx);
    VectorValue* CreateAngle3(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc);
    VectorValue* CreateAngle3(
        PropItemVec* items, const char* key, luabind::object object, const char* name, float mn, float mx, float inc, int decim);
    //			RTextValue* 		CreateName		    (PropItemVec* items, const char* key, luabind::object object, const char*
    //name,
    // ListItem* owner);
    //			RTextValue* 		CreateNameCB		(PropItemVec* items, const char* key, luabind::object object, const char*
    //name,
    // TOnDrawTextEvent=0, RTextValue::TOnBeforeEditEvent=0, RTextValue::TOnAfterEditEvent=0
};
