////////////////////////////////////////////////////////////////////////////
//	Module 		: script_properties_list_helper.cpp
//	Created 	: 14.07.2004
//  Modified 	: 14.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script properties list helper
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_properties_list_helper.h"
#include "script_token_list.h"
#include "xrServer_Object_Base.h"
#include "script_value_container_impl.h"
#include "script_value_wrapper.h"
#include "script_space.h"

CSE_Abstract *owner				(luabind::object object)
{
	CSE_Abstract			*result = luabind::object_cast<CSE_Abstract*>(object);
	R_ASSERT				(result);
	return					(result);
}

template <typename T>
struct CWrapHelper {
	typedef T result_type;
	template <bool a>
	static T	*wrap_value				(luabind::object object, LPCSTR name)
	{
		CScriptValueWrapper<T>	*value = xr_new<CScriptValueWrapper<T> >(object,name);
		owner(object)->add		(value);
		return					(value->value());
	}

	template <>
	static T	*wrap_value<true>		(luabind::object object, LPCSTR name)
	{
		return					(luabind::object_cast<T*>(object[name]));
	}
};

template <>
struct CWrapHelper<bool> {
	typedef BOOL result_type;
	template <bool a>
	static BOOL	*wrap_value				(luabind::object object, LPCSTR name)
	{
		CScriptValueWrapper<bool>	*value = xr_new<CScriptValueWrapper<bool> >(object,name);
		owner(object)->add			(value);
		return						(value->value());
	}

	template <bool a>
	static BOOL	*wrap_value				(luabind::object object, luabind::object table, LPCSTR name)
	{
		CScriptValueWrapper<bool>	*value = xr_new<CScriptValueWrapper<bool> >(table,name);
		owner(object)->add			(value);
		return						(value->value());
	}
};

template <typename T>
typename CWrapHelper<T>::result_type	*wrap_value		(luabind::object object, LPCSTR name)
{
	return						(CWrapHelper<T>::wrap_value<
		is_class<T>::result &&
		!object_type_traits::is_same<shared_str,T>::value
	>(object,name));
}

template <typename T>
typename CWrapHelper<T>::result_type	*wrap_value		(luabind::object object, luabind::object table, LPCSTR name)
{
	return						(CWrapHelper<T>::wrap_value<
		is_class<T>::result &&
		!object_type_traits::is_same<shared_str,T>::value
	>(object,table,name));
}

bool CScriptPropertiesListHelper::FvectorRDOnAfterEdit	(PropValue* sender,  Fvector& edit_val)
{
	return PHelper().FvectorRDOnAfterEdit(sender,edit_val);
}

void CScriptPropertiesListHelper::FvectorRDOnBeforeEdit	(PropValue* sender, Fvector& edit_val)
{
	PHelper().FvectorRDOnBeforeEdit(sender,edit_val);
}

/*
void CScriptPropertiesListHelper::FvectorRDOnDraw		(PropValue* sender)
{
	shared_str		temp;
	PHelper().FvectorRDOnDraw(sender,temp);
	draw_val	= *temp;
}
*/

bool CScriptPropertiesListHelper::floatRDOnAfterEdit	(PropValue* sender,  float&	 edit_val)
{
	return PHelper().floatRDOnAfterEdit(sender,edit_val);
}

void CScriptPropertiesListHelper::floatRDOnBeforeEdit	(PropValue* sender,  float&	 edit_val)
{
	PHelper().floatRDOnBeforeEdit(sender,edit_val);
}

/*
void CScriptPropertiesListHelper::floatRDOnDraw			(PropValue* sender,  LPCSTR& draw_val)
{
	shared_str		temp;
	PHelper().floatRDOnDraw(sender,temp);
	draw_val	= *temp;
}
*/

bool CScriptPropertiesListHelper::NameAfterEdit			(PropValue* sender,  LPCSTR& edit_val)
{
	shared_str		temp;
	bool result		= PHelper().NameAfterEdit(sender,temp);
	edit_val		= *temp;
	return result;
}

void CScriptPropertiesListHelper::NameBeforeEdit		(PropValue* sender,  LPCSTR& edit_val)
{
	shared_str		temp;
	PHelper().NameBeforeEdit(sender,temp);
	edit_val	= *temp;
}

/*
void CScriptPropertiesListHelper::NameDraw				(PropValue* sender,  LPCSTR& draw_val)
{
	shared_str		temp;
	PHelper().NameDraw(sender,temp);
	draw_val	= *temp;
}
*/

CaptionValue*	CScriptPropertiesListHelper::CreateCaption	(PropItemVec* items, LPCSTR key, LPCSTR val)
{
	return		(PHelper().CreateCaption(*items,key,val));
}

CanvasValue*	CScriptPropertiesListHelper::CreateCanvas	(PropItemVec* items, LPCSTR key, LPCSTR val, int height)
{
	return		(PHelper().CreateCanvas(*items,key,val,height));
}

ButtonValue*	CScriptPropertiesListHelper::CreateButton	(PropItemVec* items, LPCSTR key, LPCSTR val, u32 flags)
{
	return		(PHelper().CreateButton(*items,key,val,flags));
}

ChooseValue*	CScriptPropertiesListHelper::CreateChoose	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode, LPCSTR path, LPCSTR fill_param, u32 sub_item_cnt)
{
	return		(PHelper().CreateChoose(*items,key,wrap_value<shared_str>(object,name),mode,path,(void*)fill_param,sub_item_cnt));
}

ChooseValue*	CScriptPropertiesListHelper::CreateChoose	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode, LPCSTR path, LPCSTR fill_param)
{
	return		(PHelper().CreateChoose(*items,key,wrap_value<shared_str>(object,name),mode,path,(void*)fill_param));
}

ChooseValue*	CScriptPropertiesListHelper::CreateChoose	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode, LPCSTR path)
{
	return		(PHelper().CreateChoose(*items,key,wrap_value<shared_str>(object,name),mode,path));
}

ChooseValue*	CScriptPropertiesListHelper::CreateChoose	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mode)
{
	return		(PHelper().CreateChoose(*items,key,wrap_value<shared_str>(object,name),mode,0));
}

//S8Value* CScriptPropertiesListHelper::CreateS8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name,  s8 mn,  s8 mx,  s8 inc)
//{
//	return		(PHelper().CreateS8(*items,key,wrap_value<s8>(object,name),mn,mx,inc));
//}
//
//S8Value* CScriptPropertiesListHelper::CreateS8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name,  s8 mn,  s8 mx)
//{
//	return		(PHelper().CreateS8(*items,key,wrap_value<s8>(object,name),mn,mx));
//}
//
//S8Value* CScriptPropertiesListHelper::CreateS8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name,  s8 mn)
//{
//	return		(PHelper().CreateS8(*items,key,wrap_value<s8>(object,name),mn));
//}
//
//S8Value* CScriptPropertiesListHelper::CreateS8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
//{
//	return		(PHelper().CreateS8(*items,key,wrap_value<s8>(object,name)));
//}
//
S16Value* CScriptPropertiesListHelper::CreateS16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s16 mn, s16 mx, s16 inc)
{
	return		(PHelper().CreateS16(*items,key,wrap_value<s16>(object,name),mn,mx,inc));
}

S16Value* CScriptPropertiesListHelper::CreateS16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s16 mn, s16 mx)
{
	return		(PHelper().CreateS16(*items,key,wrap_value<s16>(object,name),mn,mx));
}

S16Value* CScriptPropertiesListHelper::CreateS16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s16 mn)
{
	return		(PHelper().CreateS16(*items,key,wrap_value<s16>(object,name),mn));
}

S16Value* CScriptPropertiesListHelper::CreateS16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateS16(*items,key,wrap_value<s16>(object,name)));
}

S32Value* CScriptPropertiesListHelper::CreateS32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s32 mn, s32 mx, s32 inc)
{
	return		(PHelper().CreateS32(*items,key,wrap_value<s32>(object,name),mn,mx,inc));
}

S32Value* CScriptPropertiesListHelper::CreateS32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s32 mn, s32 mx)
{
	return		(PHelper().CreateS32(*items,key,wrap_value<s32>(object,name),mn,mx));
}

S32Value* CScriptPropertiesListHelper::CreateS32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, s32 mn)
{
	return		(PHelper().CreateS32(*items,key,wrap_value<s32>(object,name),mn));
}

S32Value* CScriptPropertiesListHelper::CreateS32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateS32(*items,key,wrap_value<s32>(object,name)));
}

U8Value* CScriptPropertiesListHelper::CreateU8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name,  u8 mn,  u8 mx,  u8 inc)
{																											   
	return		(PHelper().CreateU8(*items,key,wrap_value<u8>(object,name),mn,mx,inc));												   
}																											   

U8Value* CScriptPropertiesListHelper::CreateU8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name,  u8 mn,  u8 mx)
{																											   
	return		(PHelper().CreateU8(*items,key,wrap_value<u8>(object,name),mn,mx));												   
}																											   

U8Value* CScriptPropertiesListHelper::CreateU8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name,  u8 mn)
{																											   
	return		(PHelper().CreateU8(*items,key,wrap_value<u8>(object,name),mn));												   
}																											   

U8Value* CScriptPropertiesListHelper::CreateU8		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{																											   
	return		(PHelper().CreateU8(*items,key,wrap_value<u8>(object,name)));
}																											   

U16Value* CScriptPropertiesListHelper::CreateU16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mn, u16 mx, u16 inc)
{																											   
	return		(PHelper().CreateU16(*items,key,wrap_value<u16>(object,name),mn,mx,inc));												   
}																											   

U16Value* CScriptPropertiesListHelper::CreateU16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mn, u16 mx)
{																											   
	return		(PHelper().CreateU16(*items,key,wrap_value<u16>(object,name),mn,mx));												   
}																											   

U16Value* CScriptPropertiesListHelper::CreateU16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mn)
{																											   
	return		(PHelper().CreateU16(*items,key,wrap_value<u16>(object,name),mn));												   
}																											   

U16Value* CScriptPropertiesListHelper::CreateU16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{																											   
	return		(PHelper().CreateU16(*items,key,wrap_value<u16>(object,name)));
}																											   

U32Value* CScriptPropertiesListHelper::CreateU32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mn, u32 mx, u32 inc)
{
	return		(PHelper().CreateU32(*items,key,wrap_value<u32>(object,name),mn,mx,inc));
}

U32Value* CScriptPropertiesListHelper::CreateU32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mn, u32 mx)
{
	return		(PHelper().CreateU32(*items,key,wrap_value<u32>(object,name),mn,mx));
}

U32Value* CScriptPropertiesListHelper::CreateU32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mn)
{
	return		(PHelper().CreateU32(*items,key,wrap_value<u32>(object,name),mn));
}

U32Value* CScriptPropertiesListHelper::CreateU32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateU32(*items,key,wrap_value<u32>(object,name)));
}

FloatValue* CScriptPropertiesListHelper::CreateFloat(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim)
{
	return		(PHelper().CreateFloat(*items,key,wrap_value<float>(object,name),mn,mx,inc,decim));
}

FloatValue* CScriptPropertiesListHelper::CreateFloat(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc)
{
	return		(PHelper().CreateFloat(*items,key,wrap_value<float>(object,name),mn,mx,inc));
}

FloatValue* CScriptPropertiesListHelper::CreateFloat(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx)
{
	return		(PHelper().CreateFloat(*items,key,wrap_value<float>(object,name),mn,mx));
}

FloatValue* CScriptPropertiesListHelper::CreateFloat(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn)
{
	return		(PHelper().CreateFloat(*items,key,wrap_value<float>(object,name),mn));
}

FloatValue* CScriptPropertiesListHelper::CreateFloat(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateFloat(*items,key,wrap_value<float>(object,name)));
}

BOOLValue* CScriptPropertiesListHelper::CreateBOOL(PropItemVec* items, LPCSTR key, luabind::object object, luabind::object table, LPCSTR name)
{
	return		(PHelper().CreateBOOL(*items,key,wrap_value<bool>(object,table,name)));
}

VectorValue* CScriptPropertiesListHelper::CreateVector(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim)
{
	return		(PHelper().CreateVector(*items,key,wrap_value<Fvector>(object,name),mn,mx,inc,decim));
}

VectorValue* CScriptPropertiesListHelper::CreateVector(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc)
{
	return		(PHelper().CreateVector(*items,key,wrap_value<Fvector>(object,name),mn,mx,inc));
}

VectorValue* CScriptPropertiesListHelper::CreateVector(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx)
{
	return		(PHelper().CreateVector(*items,key,wrap_value<Fvector>(object,name),mn,mx));
}

VectorValue* CScriptPropertiesListHelper::CreateVector(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn)
{
	return		(PHelper().CreateVector(*items,key,wrap_value<Fvector>(object,name),mn));
}

VectorValue* CScriptPropertiesListHelper::CreateVector(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateVector(*items,key,wrap_value<Fvector>(object,name)));
}

Flag8Value*		CScriptPropertiesListHelper::CreateFlag8	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask, LPCSTR c0, LPCSTR c1, u32 flags)
{
	return		(PHelper().CreateFlag8(*items,key,wrap_value<Flags8>(object,name),mask,c0,c1,flags));
}

Flag8Value*		CScriptPropertiesListHelper::CreateFlag8	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask, LPCSTR c0, LPCSTR c1)
{
	return		(PHelper().CreateFlag8(*items,key,wrap_value<Flags8>(object,name),mask,c0,c1));
}

Flag8Value*		CScriptPropertiesListHelper::CreateFlag8	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask, LPCSTR c0)
{
	return		(PHelper().CreateFlag8(*items,key,wrap_value<Flags8>(object,name),mask,c0));
}

Flag8Value*		CScriptPropertiesListHelper::CreateFlag8	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u8 mask)
{
	return		(PHelper().CreateFlag8(*items,key,wrap_value<Flags8>(object,name),mask));
}

Flag16Value*	CScriptPropertiesListHelper::CreateFlag16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask, LPCSTR c0, LPCSTR c1, u32 flags)
{
	return		(PHelper().CreateFlag16(*items,key,wrap_value<Flags16>(object,name),mask,c0,c1,flags));
}

Flag16Value*	CScriptPropertiesListHelper::CreateFlag16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask, LPCSTR c0, LPCSTR c1)
{
	return		(PHelper().CreateFlag16(*items,key,wrap_value<Flags16>(object,name),mask,c0,c1));
}

Flag16Value*	CScriptPropertiesListHelper::CreateFlag16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask, LPCSTR c0)
{
	return		(PHelper().CreateFlag16(*items,key,wrap_value<Flags16>(object,name),mask,c0));
}

Flag16Value*	CScriptPropertiesListHelper::CreateFlag16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u16 mask)
{
	return		(PHelper().CreateFlag16(*items,key,wrap_value<Flags16>(object,name),mask));
}

Flag32Value*	CScriptPropertiesListHelper::CreateFlag32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask, LPCSTR c0, LPCSTR c1, u32 flags)
{
	return		(PHelper().CreateFlag32(*items,key,wrap_value<Flags32>(object,name),mask,c0,c1,flags));
}

Flag32Value*	CScriptPropertiesListHelper::CreateFlag32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask, LPCSTR c0, LPCSTR c1)
{
	return		(PHelper().CreateFlag32(*items,key,wrap_value<Flags32>(object,name),mask,c0,c1));
}

Flag32Value*	CScriptPropertiesListHelper::CreateFlag32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask, LPCSTR c0)
{
	return		(PHelper().CreateFlag32(*items,key,wrap_value<Flags32>(object,name),mask,c0));
}

Flag32Value*	CScriptPropertiesListHelper::CreateFlag32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, u32 mask)
{
	return		(PHelper().CreateFlag32(*items,key,wrap_value<Flags32>(object,name),mask));
}

Token8Value*	CScriptPropertiesListHelper::CreateToken8	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptTokenList* token)
{
	return		(PHelper().CreateToken8(*items,key,wrap_value<u8>(object,name),&*token->tokens().begin()));
}

Token16Value*	CScriptPropertiesListHelper::CreateToken16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptTokenList* token)
{
	return		(PHelper().CreateToken16(*items,key,wrap_value<u16>(object,name),&*token->tokens().begin()));
}

Token32Value*	CScriptPropertiesListHelper::CreateToken32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptTokenList* token)
{
	return		(PHelper().CreateToken32(*items,key,wrap_value<u32>(object,name),&*token->tokens().begin()));
}

/*
RToken8Value*	CScriptPropertiesListHelper::CreateRToken8	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, xr_rtoken* token, u32 t_cnt)
{
	return		(PHelper().CreateRToken8(*items,key,wrap_value<shared_str>(object,name),token));
}

RToken16Value*	CScriptPropertiesListHelper::CreateRToken16	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, RTokenVec* token)
{
	return		(PHelper().CreateRToken16(*items,key,wrap_value<shared_str>(object,name),token));
}

RToken32Value*	CScriptPropertiesListHelper::CreateRToken32	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, RTokenVec* token)
{
	return		(PHelper().CreateRToken32(*items,key,wrap_value<shared_str>(object,name),token));
}
*/

RListValue* CScriptPropertiesListHelper::CreateRList	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, CScriptRTokenList *tokens)
{
	return		(PHelper().CreateRList(*items,key,wrap_value<shared_str>(object,name),&*tokens->tokens().begin(),tokens->size()));
}

U32Value*  CScriptPropertiesListHelper::CreateColor	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateColor(*items,key,wrap_value<u32>(object,name)));
}

ColorValue*	CScriptPropertiesListHelper::CreateFColor	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateFColor(*items,key,wrap_value<Fcolor>(object,name)));
}

VectorValue* CScriptPropertiesListHelper::CreateVColor(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateVColor(*items,key,wrap_value<Fvector>(object,name)));
}

RTextValue*	CScriptPropertiesListHelper::CreateRText	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateRText(*items,key,wrap_value<shared_str>(object,name)));
}

FloatValue* CScriptPropertiesListHelper::CreateTime		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx)
{
	return		(PHelper().CreateTime(*items,key,wrap_value<float>(object,name),mn,mx));
}

FloatValue* CScriptPropertiesListHelper::CreateTime		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn)
{
	return		(PHelper().CreateTime(*items,key,wrap_value<float>(object,name),mn));
}

FloatValue* CScriptPropertiesListHelper::CreateTime		(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateTime(*items,key,wrap_value<float>(object,name)));
}

FloatValue* CScriptPropertiesListHelper::CreateAngle	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim)
{
	return		(PHelper().CreateAngle(*items,key,wrap_value<float>(object,name),mn,mx,inc,decim));
}

FloatValue* CScriptPropertiesListHelper::CreateAngle	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc)
{
	return		(PHelper().CreateAngle(*items,key,wrap_value<float>(object,name),mn,mx,inc));
}

FloatValue* CScriptPropertiesListHelper::CreateAngle	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx)
{
	return		(PHelper().CreateAngle(*items,key,wrap_value<float>(object,name),mn,mx));
}

FloatValue* CScriptPropertiesListHelper::CreateAngle	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn)
{
	return		(PHelper().CreateAngle(*items,key,wrap_value<float>(object,name),mn));
}

FloatValue* CScriptPropertiesListHelper::CreateAngle	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateAngle(*items,key,wrap_value<float>(object,name)));
}

VectorValue* CScriptPropertiesListHelper::CreateAngle3	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc, int decim)
{
	return		(PHelper().CreateAngle3(*items,key,wrap_value<Fvector>(object,name),mn,mx,inc,decim));
}

VectorValue* CScriptPropertiesListHelper::CreateAngle3	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx, float inc)
{
	return		(PHelper().CreateAngle3(*items,key,wrap_value<Fvector>(object,name),mn,mx,inc));
}

VectorValue* CScriptPropertiesListHelper::CreateAngle3	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn, float mx)
{
	return		(PHelper().CreateAngle3(*items,key,wrap_value<Fvector>(object,name),mn,mx));
}

VectorValue* CScriptPropertiesListHelper::CreateAngle3	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name, float mn)
{
	return		(PHelper().CreateAngle3(*items,key,wrap_value<Fvector>(object,name),mn));
}

VectorValue* CScriptPropertiesListHelper::CreateAngle3	(PropItemVec* items, LPCSTR key, luabind::object object, LPCSTR name)
{
	return		(PHelper().CreateAngle3(*items,key,wrap_value<Fvector>(object,name)));
}
