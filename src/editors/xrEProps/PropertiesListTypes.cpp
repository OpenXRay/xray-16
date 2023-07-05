#include "stdafx.h"
#pragma hdrstop

#include "../../xrServerEntities/PropertiesListTypes.h"
//------------------------------------------------------------------------------

xr_string ShortcutValue::GetDrawText(TOnDrawTextEvent)
{
	xr_string txt; // = ToStr(MxShortCutToText(value->hotkey)).c_str();
	if (value->key == 0)
	{
		txt.append("none");
		return txt;
	}
	if (value->ext.test(xr_shortcut::flCtrl))
	{
		txt.append("Ctrl+");
	}
	if (value->ext.test(xr_shortcut::flShift))
	{
		txt.append("Shift+");
	}
	if (value->ext.test(xr_shortcut::flAlt))
	{
		txt.append("Alt+");
	}
	if (value->key >= 'A' && value->key <= 'Z')
	{
		txt.append(1, (char)value->key);
	}
	else if (value->key >= '0' && value->key <= '9')
	{
		txt.append(1, (char)value->key);
	}
	else
	{
		switch (value->key)
		{
		case VK_ADD:
			txt.append("Numpad+");
			break;
		case VK_SUBTRACT:
			txt.append("Numpad-");
			break;
		case VK_MULTIPLY:
			txt.append("Numpad*");
			break;
		case VK_DIVIDE:
			txt.append("Numpad/");
			break;
		case VK_OEM_PLUS:
			txt.append("+");
			break;
		case VK_OEM_MINUS:
			txt.append("-");
			break;
		case VK_OEM_1:
			txt.append(";:");
			break;
		case VK_OEM_COMMA:
			txt.append(",");
			break;
		case VK_OEM_PERIOD:
			txt.append(".");
			break;
		case VK_OEM_2:
			txt.append("/?");
			break;
		case VK_OEM_4:
			txt.append("{[");
			break;
		case VK_OEM_5:
			txt.append("\\|");
			break;
		case VK_OEM_6:
			txt.append("]}");
			break;
		case VK_OEM_7:
			txt.append("\"\'");
			break;
		case VK_SPACE:
			txt.append("Space");
			break;
		case VK_CANCEL:
			txt.append("Scroll Lock");
			break;
		case VK_RETURN:
			txt.append("Enter");
			break;
			////////////////////////////
		case VK_LEFT:
			txt.append("Left");
			break;
		case VK_RIGHT:
			txt.append("Right");
			break;
		case VK_UP:
			txt.append("Up");
			break;
		case VK_DOWN:
			txt.append("Down");
			break;
		case VK_NUMPAD0:
			txt.append("Numpad0");
			break;
		case VK_NUMPAD1:
			txt.append("Numpad1");
			break;
		case VK_NUMPAD2:
			txt.append("Numpad2");
			break;
		case VK_NUMPAD3:
			txt.append("Numpad3");
			break;
		case VK_NUMPAD4:
			txt.append("Numpad4");
			break;
		case VK_NUMPAD5:
			txt.append("Numpad5");
			break;
		case VK_NUMPAD6:
			txt.append("Numpad6");
			break;
		case VK_NUMPAD7:
			txt.append("Numpad7");
			break;
		case VK_NUMPAD8:
			txt.append("Numpad8");
			break;
		case VK_NUMPAD9:
			txt.append("Numpad9");
			break;
		case VK_F1:
			txt.append("F1");
			break;
		case VK_F2:
			txt.append("F2");
			break;
		case VK_F3:
			txt.append("F3");
			break;
		case VK_F4:
			txt.append("F4");
			break;
		case VK_F5:
			txt.append("F5");
			break;
		case VK_F6:
			txt.append("F6");
			break;
		case VK_F7:
			txt.append("F7");
			break;
		case VK_F8:
			txt.append("F8");
			break;
		case VK_F9:
			txt.append("F9");
			break;
		case VK_F10:
			txt.append("F10");
			break;
		case VK_F11:
			txt.append("F11");
			break;
		case VK_F12:
			txt.append("F12");
			break;
		case VK_DELETE:
			txt.append("Delete");
			break;

		default:
			VERIFY(0);
			break;
		}
	}
	return txt;
}

xr_string GameTypeValue::GetDrawText(TOnDrawTextEvent)
{
	string512 str;
	xr_sprintf(str, sizeof(str), "%s%s%s%s%s",
			   GetValue().MatchType(eGameIDSingle) ? "Single " : "",
			   GetValue().MatchType(eGameIDDeathmatch) ? "DM " : "",
			   GetValue().MatchType(eGameIDTeamDeathmatch) ? "TDM " : "",
			   GetValue().MatchType(eGameIDArtefactHunt) ? "AH " : "",
			   GetValue().MatchType(eGameIDCaptureTheArtefact) ? "CTA" : "");
	return xr_string(str);
}
