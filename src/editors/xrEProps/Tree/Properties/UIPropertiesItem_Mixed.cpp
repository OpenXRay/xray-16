#include "stdafx.h"

template <typename T>
inline bool MixedNumeric(PropItem *item, bool &change)
{
	change = false;
	NumericValue<T> *V = dynamic_cast<NumericValue<T> *>(item->GetFrontValue());
	if (!V)
		return false;
	T value = *V->value;
	item->BeforeEdit<NumericValue<T>, T>(value);

	if (item->AfterEdit<NumericValue<T>, T>(value))
	{
		change = item->ApplyValue<NumericValue<T>, T>(value);
	}
	return true;
}

template <class T>
BOOL MixedFlag(PropItem *prop, bool &change)
{
	FlagValue<_flags<T>> *V = dynamic_cast<FlagValue<_flags<T>> *>(prop->GetFrontValue());
	if (!V)
		return FALSE;
	_flags<T> new_val = V->GetValue();

	prop->BeforeEdit<FlagValue<_flags<T>>, _flags<T>>(new_val);
	if (prop->AfterEdit<FlagValue<_flags<T>>, _flags<T>>(new_val))
		change = prop->ApplyValue<FlagValue<_flags<T>>, _flags<T>>(new_val);
	return TRUE;
}

template <class T>
BOOL MixedToken(PropItem *prop, bool &change)
{
	TokenValue<T> *V = dynamic_cast<TokenValue<T> *>(prop->GetFrontValue());
	if (!V)
		return FALSE;
	T edit_value = V->GetValue();
	prop->BeforeEdit<TokenValue<T>, T>(edit_value);
	if (prop->AfterEdit<TokenValue<T>, T>(edit_value))
		change = prop->ApplyValue<TokenValue<T>, T>(edit_value);
	return TRUE;
}

template <class T>
BOOL MixedRToken(PropItem *prop, bool &change)
{
	RTokenValue<T> *V = dynamic_cast<RTokenValue<T> *>(prop->GetFrontValue());
	if (!V)
		return FALSE;
	T edit_value = V->GetValue();
	prop->BeforeEdit<RTokenValue<T>, T>(edit_value);
	if (prop->AfterEdit<RTokenValue<T>, T>(edit_value))
		change = prop->ApplyValue<RTokenValue<T>, T>(edit_value);
	return TRUE;
}

void UIPropertiesItem::RemoveMixed()
{
	PropItem* node = PItem;
	EPropType type = node->Type();
	bool change = false;

	auto Modified = [&]()
	{
		PropertiesFrom->Modified();
	};

	switch (type)
	{
	case PROP_SHORTCUT:
	{
		ShortcutValue *V = dynamic_cast<ShortcutValue*>(node->GetFrontValue());
		R_ASSERT(V);
		xr_shortcut val = *V->value;

		if (V->ApplyValue(val))
			change = true;
	}
	break;

	case PROP_CHOOSE:
	{
		ChooseValue *V = dynamic_cast<ChooseValue*>(node->GetFrontValue());
		VERIFY(V);
		shared_str edit_val = V->GetValue();

		if (!edit_val.size())
			edit_val = V->m_StartPath;

		node->BeforeEdit<ChooseValue, shared_str>(edit_val);

		if (node->AfterEdit<ChooseValue, shared_str>(edit_val))
		{
			if (node->ApplyValue<ChooseValue, shared_str>(edit_val))
				change = true;
		}
	}
	break;

	case PROP_NUMERIC:
	{
		if (!MixedNumeric<u32>(node, change))
			if (!MixedNumeric<float>(node, change))
				if (!MixedNumeric<u8>(node, change))
					if (!MixedNumeric<s8>(node, change))
						if (!MixedNumeric<u16>(node, change))
							if (!MixedNumeric<u16>(node, change))
								if (!MixedNumeric<s32>(node, change))
									R_ASSERT(false);
	}
	break;

	case PROP_BOOLEAN:
	{
		BOOLValue *V = dynamic_cast<BOOLValue*>(node->GetFrontValue());
		VERIFY(V);
		BOOL val = V->GetValue();
		node->BeforeEdit<BOOLValue, BOOL>(val);

		if (node->AfterEdit<BOOLValue, BOOL>(val))
		{
			if (node->ApplyValue<BOOLValue, BOOL>(val))
				change = true;
		}
	}
	break;

	case PROP_FLAG:
	{
		if (!MixedFlag<u8>(node, change))
			if (!MixedFlag<u16>(node, change))
				if (!MixedFlag<u32>(node, change))
					R_ASSERT(false);
	}
	break;

	case PROP_VECTOR:
	{
		VectorValue *V = dynamic_cast<VectorValue*>(node->GetFrontValue());
		R_ASSERT(V);
		Fvector edit_val = V->GetValue();
		node->BeforeEdit<VectorValue, Fvector>(edit_val);

		if (node->AfterEdit<VectorValue, Fvector>(edit_val))
		{
			if (node->ApplyValue<VectorValue, Fvector>(edit_val))
				change = true;
		}
	}
	break;

	case PROP_TOKEN:
	{
		if (!MixedToken<u8>(node, change))
			if (!MixedToken<u16>(node, change))
				if (!MixedToken<u32>(node, change))
					R_ASSERT(false);
	}
	break;

	case PROP_RTOKEN:
	{
		if (!MixedRToken<u8>(node, change))
			if (!MixedRToken<u16>(node, change))
				if (!MixedRToken<u32>(node, change))
					R_ASSERT(false);
	}
	break;

	case PROP_RLIST:
	{
		RListValue *V = dynamic_cast<RListValue*>(node->GetFrontValue());
		R_ASSERT(V);
		LPCSTR edit_value = V->value ? V->value->c_str() : nullptr;
		int index = 0;
		const char *InTokens[256];

		for (int i = 0; i < V->item_count; i++)
		{
			if (V->items[i] == edit_value)
				index = i;
		}

		if (node->AfterEdit<RListValue, shared_str>(V->items[index]))
			if (node->ApplyValue<RListValue, shared_str>(V->items[index]))
				change = true;
	}
	break;

	case PROP_COLOR:
	{
		U32Value *V = dynamic_cast<U32Value*>(node->GetFrontValue());
		R_ASSERT(V);
		u32 edit_val = V->GetValue();
		node->BeforeEdit<U32Value, u32>(edit_val);

		if (node->AfterEdit<U32Value, u32>(edit_val))
		{
			if (node->ApplyValue<U32Value, u32>(edit_val))
				change = true;
		}
	}
	break;

	case PROP_FCOLOR:
	{
		ColorValue *V = dynamic_cast<ColorValue*>(node->GetFrontValue());
		R_ASSERT(V);
		Fcolor edit_val = V->GetValue();
		node->BeforeEdit<ColorValue, Fcolor>(edit_val);

		if (node->AfterEdit<ColorValue, Fcolor>(edit_val))
		{
			if (node->ApplyValue<ColorValue, Fcolor>(edit_val))
				change = true;
		}
	}
	break;

	case PROP_VCOLOR:
	{
		VectorValue *V = dynamic_cast<VectorValue*>(node->GetFrontValue());
		R_ASSERT(V);
		Fvector edit_val = V->GetValue();
		node->BeforeEdit<VectorValue, Fvector>(edit_val);

		if (node->AfterEdit<VectorValue, Fvector>(edit_val))
		{
			if (node->ApplyValue<VectorValue, Fvector>(edit_val))
				change = true;
		}
	}
	break;

	case PROP_RTEXT:
	case PROP_STEXT:
	case PROP_CTEXT:
	{
		if (CTextValue* V1 = dynamic_cast<CTextValue*>(node->GetFrontValue()))
		{
			xr_string out = V1->GetValue();
			if (node->AfterEdit<CTextValue, xr_string>(out))
			{
				if (node->ApplyValue<CTextValue, LPCSTR>(out.c_str()))
					change = true;
			}
		}
		else if (RTextValue* V2 = dynamic_cast<RTextValue*>(node->GetFrontValue()))
		{
			shared_str out = V2->GetValue();
			if (node->AfterEdit<RTextValue, shared_str>(out))
			{
				if (node->ApplyValue<RTextValue, shared_str>(out))
					change = true;
			}
		}
		else if (STextValue* V3 = dynamic_cast<STextValue*>(node->GetFrontValue()))
		{
			xr_string out = V3->GetValue();
			if (node->AfterEdit<STextValue, xr_string>(out))
			{
				if (node->ApplyValue<STextValue, xr_string>(out))
					change = true;
			}
		}
		else
			R_ASSERT(false);
	}
	break;

	case PROP_CLIST:
	{
		CListValue *V = dynamic_cast<CListValue*>(node->GetFrontValue());
		R_ASSERT(V);
		LPCSTR edit_value = V->value;
		int index = 0;

		for (int i = 0; i < V->item_count; i++)
		{
			if (V->items[i] == edit_value)
				index = i;
		}

		if (node->AfterEdit<CListValue, xr_string>(V->items[index]))
		{
			if (node->ApplyValue<CListValue, LPCSTR>(V->items[index].c_str()))
				change = true;
		}
	}
	break;

	case PROP_SH_TOKEN:
	{
		TokenValueSH *V = dynamic_cast<TokenValueSH*>(node->GetFrontValue());
		R_ASSERT(V);
		u32 edit_value = V->GetValue();
		node->BeforeEdit<TokenValueSH, u32>(edit_value);

		if (node->AfterEdit<TokenValueSH, u32>(edit_value))
		{
			if (node->ApplyValue<TokenValueSH, u32>(edit_value))
				change = true;
		}
	}
	break;

	case PROP_TEXTURE2:
	{
		CTextValue *T = dynamic_cast<CTextValue*>(node->GetFrontValue());
		R_ASSERT(T);
		xr_string edit_val = T->GetValue();
		node->BeforeEdit<CTextValue, xr_string>(edit_val);

		if (node->AfterEdit<CTextValue, xr_string>(edit_val))
		{
			if (node->ApplyValue<CTextValue, LPCSTR>(edit_val.c_str()))
				change = true;
		}
	}
	break;

	case PROP_GAMETYPE:
	{
		GameTypeValue *V = dynamic_cast<GameTypeValue *>(node->GetFrontValue());
		R_ASSERT(V);
		ImGui::Text(node->GetDrawText().c_str());
		auto value = V->GetValue();
		node->BeforeEdit<GameTypeValue, GameTypeChooser>(value);

		if (node->AfterEdit<GameTypeValue, GameTypeChooser>(value))
		{
			if (node->ApplyValue<GameTypeValue, GameTypeChooser>(value))
				change = true;
		}
	}
	break;

	default:
		return;
		break;
	}

#pragma TODO("TSMP: какой то странный тут код")

	if (change)
	{
		Modified();
	}
	else
	{
		Modified();
		PItem->m_Flags.set(PropItem::flIgnoreMixed, 1);
	}
}
