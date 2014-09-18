#include "stdafx.h"

#include "HudSound.h"

float psHUDSoundVolume			= 1.0f;
float psHUDStepSoundVolume		= 1.0f;
void InitHudSoundSettings()
{
	psHUDSoundVolume		= pSettings->r_float("hud_sound", "hud_sound_vol_k");
	psHUDStepSoundVolume	= pSettings->r_float("hud_sound", "hud_step_sound_vol_k");
}

void HUD_SOUND_ITEM::LoadSound(	LPCSTR section, LPCSTR line, 
							HUD_SOUND_ITEM& hud_snd, int type)
{
	hud_snd.m_activeSnd		= NULL;
	hud_snd.sounds.clear	();

	string256	sound_line;
	xr_strcpy		(sound_line,line);
	int k=0;
	while( pSettings->line_exist(section, sound_line) ){
		hud_snd.sounds.push_back( SSnd() );
		SSnd& s = hud_snd.sounds.back();

		LoadSound	(section, sound_line, s.snd, type, &s.volume, &s.delay);
		xr_sprintf		(sound_line,"%s%d",line,++k);
	}//while
}

void  HUD_SOUND_ITEM::LoadSound(LPCSTR section, 
								LPCSTR line, 
								ref_sound& snd, 
								int type,
								float* volume, 
								float* delay)
{
	LPCSTR str = pSettings->r_string(section, line);
	string256 buf_str;

	int	count = _GetItemCount	(str);
	R_ASSERT(count);

	_GetItem(str, 0, buf_str);
	snd.create(buf_str, st_Effect,type);


	if(volume != NULL)
	{
		*volume = 1.f;
		if(count>1)
		{
			_GetItem (str, 1, buf_str);
			if(xr_strlen(buf_str)>0)
				*volume = (float)atof(buf_str);
		}
	}

	if(delay != NULL)
	{
		*delay = 0;
		if(count>2)
		{
			_GetItem (str, 2, buf_str);
			if(xr_strlen(buf_str)>0)
				*delay = (float)atof(buf_str);
		}
	}
}

void HUD_SOUND_ITEM::DestroySound(HUD_SOUND_ITEM& hud_snd)
{
	xr_vector<SSnd>::iterator it = hud_snd.sounds.begin();
	for(;it!=hud_snd.sounds.end();++it)
		(*it).snd.destroy();
	hud_snd.sounds.clear	();
	
	hud_snd.m_activeSnd		= NULL;
}

void HUD_SOUND_ITEM::PlaySound(	HUD_SOUND_ITEM&		hud_snd,
								const Fvector&	position,
								const CObject*	parent,
								bool			b_hud_mode,
								bool			looped,
								u8 index)
{
	if (hud_snd.sounds.empty())	return;

	hud_snd.m_activeSnd			= NULL;
	StopSound					(hud_snd);

	u32 flags = b_hud_mode?sm_2D:0;
	if(looped)
		flags |= sm_Looped;

	if(index==u8(-1))
		index = (u8)Random.randI(hud_snd.sounds.size());

	hud_snd.m_activeSnd = &hud_snd.sounds[ index ];
	

	hud_snd.m_activeSnd->snd.play_at_pos(	const_cast<CObject*>(parent),
											flags&sm_2D?Fvector().set(0,0,0):position,
											flags,
											hud_snd.m_activeSnd->delay);

	hud_snd.m_activeSnd->snd.set_volume		(hud_snd.m_activeSnd->volume * b_hud_mode?psHUDSoundVolume:1.0f);
}

void HUD_SOUND_ITEM::StopSound(HUD_SOUND_ITEM& hud_snd)
{
	xr_vector<SSnd>::iterator it = hud_snd.sounds.begin();
	for(;it!=hud_snd.sounds.end();++it)
		(*it).snd.stop		();
	hud_snd.m_activeSnd		= NULL;
}

//----------------------------------------------------------
HUD_SOUND_COLLECTION::~HUD_SOUND_COLLECTION()
{
	xr_vector<HUD_SOUND_ITEM>::iterator it		= m_sound_items.begin();
	xr_vector<HUD_SOUND_ITEM>::iterator it_e	= m_sound_items.end();

	for(;it!=it_e;++it)
	{
		HUD_SOUND_ITEM::StopSound		(*it);
		HUD_SOUND_ITEM::DestroySound	(*it);
	}

	m_sound_items.clear();
}

HUD_SOUND_ITEM* HUD_SOUND_COLLECTION::FindSoundItem(LPCSTR alias, bool b_assert)
{
	xr_vector<HUD_SOUND_ITEM>::iterator it	= std::find(m_sound_items.begin(),m_sound_items.end(),alias);
	
	if(it!=m_sound_items.end())
		return &*it;
	else{
		R_ASSERT3(!b_assert,"sound item not found in collection", alias);
		return NULL;
	}
}

void HUD_SOUND_COLLECTION::PlaySound(	LPCSTR alias, 
										const Fvector& position,
										const CObject* parent,
										bool hud_mode,
										bool looped,
										u8 index)
{
	xr_vector<HUD_SOUND_ITEM>::iterator it		= m_sound_items.begin();
	xr_vector<HUD_SOUND_ITEM>::iterator it_e	= m_sound_items.end();
	for(;it!=it_e;++it)
	{
		if(it->m_b_exclusive)
			HUD_SOUND_ITEM::StopSound	(*it);
	}


	HUD_SOUND_ITEM* snd_item		= FindSoundItem(alias, true);
	HUD_SOUND_ITEM::PlaySound		(*snd_item, position, parent, hud_mode, looped, index);
}

void HUD_SOUND_COLLECTION::StopSound(LPCSTR alias)
{
	HUD_SOUND_ITEM* snd_item		= FindSoundItem(alias, true);
	HUD_SOUND_ITEM::StopSound		(*snd_item);
}

void HUD_SOUND_COLLECTION::SetPosition(LPCSTR alias, const Fvector& pos)
{
	HUD_SOUND_ITEM* snd_item		= FindSoundItem(alias, true);
	if(snd_item->playing())
		snd_item->set_position		(pos);
}

void HUD_SOUND_COLLECTION::StopAllSounds()
{
	xr_vector<HUD_SOUND_ITEM>::iterator it		= m_sound_items.begin();
	xr_vector<HUD_SOUND_ITEM>::iterator it_e	= m_sound_items.end();

	for(;it!=it_e;++it)
	{
		HUD_SOUND_ITEM::StopSound	(*it);
	}
}

void HUD_SOUND_COLLECTION::LoadSound(	LPCSTR section, 
										LPCSTR line,
										LPCSTR alias,
										bool exclusive,
										int type)
{
	R_ASSERT					(NULL==FindSoundItem(alias, false));
	m_sound_items.resize		(m_sound_items.size()+1);
	HUD_SOUND_ITEM& snd_item	= m_sound_items.back();
	HUD_SOUND_ITEM::LoadSound	(section, line, snd_item, type);
	snd_item.m_alias			= alias;
	snd_item.m_b_exclusive		= exclusive;
}
