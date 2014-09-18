////////////////////////////////////////////////////////////////////////////
//	Module 		: script_properties_list_helper.h
//	Created 	: 14.07.2004
//  Modified 	: 14.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script properties list helper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../xrcdb/xrCDB.h"
#ifndef XRGAME_EXPORTS
#	include "../xrSound/Sound.h"
#endif
#include "xrEProps.h"
#include "script_export_space.h"
#include "script_space.h"
#include "script_rtoken_list.h"

class CScriptTokenList;

struct CScriptPropertiesListHelper {
public:
			bool 				FvectorRDOnAfterEdit(PropValue* sender,  Fvector& edit_val);
			void 				FvectorRDOnBeforeEdit(PropValue* sender, Fvector& edit_val);
//			void 				FvectorRDOnDraw		(PropValue* sender,  LPCSTR& draw_val);
			bool 				floatRDOnAfterEdit	(PropValue* sender,  float&	 edit_val);
			void 				floatRDOnBeforeEdit	(PropValue* sender,  float&	 edit_val);
//			void 				floatRDOnDraw		(PropValue* sender,  LPCSTR& draw_val);
// name edit
			bool 				NameAfterEdit		(PropValue* sender,  LPCSTR& edit_val);
			void				NameBeforeEdit		(PropValue* sender,  LPCSTR& edit_val);
//			void 				NameDraw			(PropValue* sender,  LPCSTR& draw_val);
public:
			CaptionValue*  		CreateCaption	    (PropItemVec* items, LPCSTR key, LPCSTR val);
			CanvasValue*		CreateCanvas	    (PropItemVec* items, LPCSTR key, LPCSTR val, int height);
			ButtonValue*		CreateButton	    (PropItemVec* items, LPCSTR key, LPCSTR val, u32 flags);
			
			ChooseValue*		CreateChoose	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode);
			ChooseValue*		CreateChoose	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode, LPCSTR path);
			ChooseValue*		CreateChoose	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode, LPCSTR path, LPCSTR fill_param);
			ChooseValue*		CreateChoose	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode, LPCSTR path, LPCSTR fill_param, u32 sub_item_cnt);
			
//			S8Value* 			CreateS8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
//			S8Value* 			CreateS8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s8 mn);
//			S8Value* 			CreateS8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s8 mn, s8 mx);
//			S8Value* 			CreateS8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s8 mn, s8 mx, s8 inc);

			S16Value* 			CreateS16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			S16Value* 			CreateS16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s16 mn);
			S16Value* 			CreateS16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s16 mn, s16 mx);
			S16Value* 			CreateS16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s16 mn, s16 mx, s16 inc);

			S32Value* 	 		CreateS32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			S32Value* 	 		CreateS32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s32 mn);
			S32Value* 	 		CreateS32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s32 mn, s32 mx);
			S32Value* 	 		CreateS32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s32 mn, s32 mx, s32 inc);

			U8Value* 			CreateU8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			U8Value* 			CreateU8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mn);
			U8Value* 			CreateU8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mn, u8 mx);
			U8Value* 			CreateU8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mn, u8 mx, u8 inc);

			U16Value* 			CreateU16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			U16Value* 			CreateU16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mn);
			U16Value* 			CreateU16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mn, u16 mx);
			U16Value* 			CreateU16		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mn, u16 mx, u16 inc);

			U32Value* 	  		CreateU32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			U32Value* 	  		CreateU32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mn);
			U32Value* 	  		CreateU32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mn, u32 mx);
			U32Value* 	  		CreateU32		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mn, u32 mx, u32 inc);

			FloatValue* 		CreateFloat		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			FloatValue* 		CreateFloat		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn);
			FloatValue* 		CreateFloat		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx);
			FloatValue* 		CreateFloat		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc);
			FloatValue* 		CreateFloat		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim);

			BOOLValue* 	  		CreateBOOL		    (PropItemVec* items, LPCSTR key, luabind::object object, luabind::object table, LPCSTR name);

			VectorValue*  	 	CreateVector	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			VectorValue*  	 	CreateVector	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn);
			VectorValue*  	 	CreateVector	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx);
			VectorValue*  	 	CreateVector	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc);
			VectorValue*  	 	CreateVector	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim);

			Flag8Value*			CreateFlag8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask);
			Flag8Value*			CreateFlag8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask, LPCSTR c0);
			Flag8Value*			CreateFlag8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask, LPCSTR c0, LPCSTR c1);
			Flag8Value*			CreateFlag8		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask, LPCSTR c0, LPCSTR c1, u32 flags);

			Flag16Value*		CreateFlag16	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask);
			Flag16Value*		CreateFlag16	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask, LPCSTR c0);
			Flag16Value*		CreateFlag16	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask, LPCSTR c0, LPCSTR c1);
			Flag16Value*		CreateFlag16	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask, LPCSTR c0, LPCSTR c1, u32 flags);

			Flag32Value*		CreateFlag32	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask);
			Flag32Value*		CreateFlag32	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask, LPCSTR c0);
			Flag32Value*		CreateFlag32	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask, LPCSTR c0, LPCSTR c1);
			Flag32Value*		CreateFlag32	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask, LPCSTR c0, LPCSTR c1, u32 flags);

			Token8Value*		CreateToken8	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptTokenList* token);
			Token16Value*		CreateToken16	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptTokenList* token);
			Token32Value*		CreateToken32	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptTokenList* token);
//			RToken8Value* 		CreateRToken8	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, RTokenVec* token);
//			RToken16Value* 		CreateRToken16	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, RTokenVec* token);
//			RToken32Value* 		CreateRToken32	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, RTokenVec* token);
//			TokenValueSH*   	CreateTokenSH	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 cnt, const TokenValueSH::Item* lst);
			RListValue* 	 	CreateRList		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptRTokenList *tokens);
			U32Value*  			CreateColor		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			ColorValue*			CreateFColor	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			VectorValue*		CreateVColor	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			RTextValue* 		CreateRText		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
//			WaveValue* 			CreateWave		    (PropItemVec* items, LPCSTR key, WaveForm* val);
			FloatValue* 		CreateTime		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			FloatValue* 		CreateTime		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn);
			FloatValue* 		CreateTime		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx);

			FloatValue* 		CreateAngle		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			FloatValue* 		CreateAngle		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn);
			FloatValue* 		CreateAngle		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx);
			FloatValue* 		CreateAngle		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc);
			FloatValue* 		CreateAngle		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim);

			VectorValue* 		CreateAngle3	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name);
			VectorValue* 		CreateAngle3	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn);
			VectorValue* 		CreateAngle3	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx);
			VectorValue* 		CreateAngle3	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc);
			VectorValue* 		CreateAngle3	    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim);
//			RTextValue* 		CreateName		    (PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, ListItem* owner);
//			RTextValue* 		CreateNameCB		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, TOnDrawTextEvent=0, RTextValue::TOnBeforeEditEvent=0, RTextValue::TOnAfterEditEvent=0);
			DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptPropertiesListHelper)
#undef script_type_list
#define script_type_list save_type_list(CScriptPropertiesListHelper)
