#include "stdafx.h"
#pragma hdrstop

#include "../../xrServerEntities/PropertiesListTypes.h"
#include "MxShortcut.hpp"
//------------------------------------------------------------------------------
            
xr_string	ShortcutValue::GetDrawText		(TOnDrawTextEvent OnDrawText)
{
    xr_string 	txt = MxShortCutToText(value->hotkey).c_str();
    if (!OnDrawText.empty())	OnDrawText(this, txt);
    return 			txt;
}

xr_string GameTypeValue::GetDrawText(TOnDrawTextEvent)
{
	string512 str;
    xr_sprintf(str,sizeof(str),"%s%s%s%s%s",
	GetValue().MatchType(eGameIDSingle)?"Single ":"",
	GetValue().MatchType(eGameIDDeathmatch)?"DM ":"",
	GetValue().MatchType(eGameIDTeamDeathmatch)?"TDM ":"",
	GetValue().MatchType(eGameIDArtefactHunt)?"AH ":"",
	GetValue().MatchType(eGameIDCaptureTheArtefact)?"CTA":""    );
	return xr_string(str);
}
