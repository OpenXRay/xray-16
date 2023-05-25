#include "stdafx.h"

#define CUSTOMOBJECT_CHUNK_PARAMS 0xF900
#define CUSTOMOBJECT_CHUNK_LOCK 0xF902
#define CUSTOMOBJECT_CHUNK_TRANSFORM 0xF903
#define CUSTOMOBJECT_CHUNK_GROUP 0xF904
#define CUSTOMOBJECT_CHUNK_MOTION 0xF905
#define CUSTOMOBJECT_CHUNK_FLAGS 0xF906
#define CUSTOMOBJECT_CHUNK_NAME 0xF907
#define CUSTOMOBJECT_CHUNK_MOTION_PARAM 0xF908

enum class SocFlags : u32
{
	flSelected = (1 << 0),
	flVisible = (1 << 1),
	flLocked = (1 << 2),
	flMotion = (1 << 3),

	flAutoKey = (1 << 30),
	flCameraView = (1 << 31),
};

CCustomObject::CCustomObject(LPVOID data, LPCSTR name)
{
	save_id = 0;
	FClassID = OBJCLASS_DUMMY;

	FParentTools = nullptr;

	if (name)
		FName = name;

	m_CO_Flags.assign(0);
	m_RT_Flags.assign(flRT_Valid | flRT_Visible);
	m_pOwnerObject = 0;
	ResetTransform();
	m_RT_Flags.set(flRT_UpdateTransform, TRUE);
	m_Motion = nullptr;
	m_MotionParams = nullptr;

	FPosition.set(0, 0, 0);
	FScale.set(1, 1, 1);
	FRotation.set(0, 0, 0);
}

CCustomObject::~CCustomObject()
{
	xr_delete(m_Motion);
	xr_delete(m_MotionParams);
}

bool CCustomObject::IsRender()
{
	Fbox bb;
	GetBox(bb);

	float distance = 0.f;
	Fvector center;

	bb.getcenter(center);
	distance = center.distance_to(EDevice.vCameraPosition);

	if (distance > bb.getradius() + EDevice.RadiusRender)
		return false;

	return ::Render->occ_visible(bb) || (Selected() && m_CO_Flags.is_any(flRenderAnyWayIfSelected | flMotion));
}

void CCustomObject::OnUpdateTransform()
{
	m_RT_Flags.set(flRT_UpdateTransform, FALSE);

	// update transform matrix
	FTransformR.setXYZi(-GetRotation().x, -GetRotation().y, -GetRotation().z);

	FTransformS.scale(GetScale());
	FTransformP.translate(GetPosition());
	FTransformRP.mul(FTransformP, FTransformR);
	FTransform.mul(FTransformRP, FTransformS);
	FITransformRP.invert(FTransformRP);
	FITransform.invert(FTransform);

	if (Motionable() && Visible() && Selected() && m_CO_Flags.is(flAutoKey))
		AnimationCreateKey(m_MotionParams->Frame());
}

void CCustomObject::Select(int flag)
{
	if (m_RT_Flags.is(flRT_Visible) && (!!m_RT_Flags.is(flRT_Selected) != flag))
	{
		m_RT_Flags.set(flRT_Selected, (flag == -1) ? (m_RT_Flags.is(flRT_Selected) ? FALSE : TRUE) : flag);
		UI->RedrawScene();
		ExecCommand(COMMAND_UPDATE_PROPERTIES);
		FParentTools->OnSelected(this);
	}
}

void CCustomObject::Show(BOOL flag)
{
	m_RT_Flags.set(flRT_Visible, flag);

	if (!m_RT_Flags.is(flRT_Visible))
		m_RT_Flags.set(flRT_Selected, FALSE);

	UI->RedrawScene();
}

BOOL CCustomObject::Editable() const
{
	BOOL b1 = m_CO_Flags.is(flObjectInGroup);
	BOOL b2 = m_CO_Flags.is(flObjectInGroupUnique);
	return !b1 || (b1 && b2);
}

bool CCustomObject::LoadLTX(CInifile &ini, LPCSTR sect_name)
{
	m_CO_Flags.assign(ini.r_u32(sect_name, "co_flags"));

	FName = ini.r_string(sect_name, "name");
	FPosition = ini.r_fvector3(sect_name, "position");
	VERIFY2(_valid(FPosition), sect_name);
	FRotation = ini.r_fvector3(sect_name, "rotation");
	VERIFY2(_valid(FRotation), sect_name);
	FScale = ini.r_fvector3(sect_name, "scale");
	VERIFY2(_valid(FScale), sect_name);

	// object motion
	if (m_CO_Flags.is(flMotion))
		m_CO_Flags.set(flMotion, FALSE);

	return true;
}

bool CCustomObject::LoadStream(IReader &F)
{
	R_ASSERT(F.find_chunk(CUSTOMOBJECT_CHUNK_FLAGS));

	if (!Core.SocSdk)
		m_CO_Flags.assign(F.r_u32());
	else
	{
		Flags32 tempFlags;
		tempFlags.assign(F.r_u32());

		m_RT_Flags.set(flRT_Selected, tempFlags.is((u32)SocFlags::flSelected));
		m_RT_Flags.set(flRT_Visible, tempFlags.is((u32)SocFlags::flVisible));
		m_RT_Flags.set(flRT_Visible, tempFlags.is((u32)SocFlags::flVisible));
		m_CO_Flags.set(flMotion, tempFlags.is((u32)SocFlags::flMotion));
		m_CO_Flags.set(flAutoKey, tempFlags.is((u32)SocFlags::flAutoKey));
		m_CO_Flags.set(flCameraView, tempFlags.is((u32)SocFlags::flCameraView));
	}

	R_ASSERT(F.find_chunk(CUSTOMOBJECT_CHUNK_NAME));
	F.r_stringZ(FName);

	if (F.find_chunk(CUSTOMOBJECT_CHUNK_TRANSFORM))
	{
		F.r_fvector3(FPosition);
		F.r_fvector3(FRotation);
		VERIFY(_valid(FRotation));
		F.r_fvector3(FScale);
	}

	// object motion
	if (F.find_chunk(CUSTOMOBJECT_CHUNK_MOTION))
	{
		m_Motion = xr_new<COMotion>();

		if (!m_Motion->Load(F))
		{
			ELog.Msg(mtError, "CustomObject: '%s' - motion has different version. Load failed.", GetName());
			xr_delete(m_Motion);
		}

		m_MotionParams = xr_new<SAnimParams>();
		m_MotionParams->Set(m_Motion);
		AnimationUpdate(m_MotionParams->Frame());
	}

	if (F.find_chunk(CUSTOMOBJECT_CHUNK_MOTION_PARAM))
	{
		m_MotionParams->t_current = F.r_float();
		AnimationUpdate(m_MotionParams->Frame());
	}

	UpdateTransform();
	return true;
}

void CCustomObject::SaveLTX(CInifile &ini, LPCSTR sect_name)
{
	ini.w_u32(sect_name, "co_flags", m_CO_Flags.get());
	ini.w_string(sect_name, "name", FName.c_str());

	ini.w_fvector3(sect_name, "position", FPosition);
	ini.w_fvector3(sect_name, "rotation", FRotation);
	ini.w_fvector3(sect_name, "scale", FScale);
}

void CCustomObject::SaveStream(IWriter &F)
{
	F.open_chunk(CUSTOMOBJECT_CHUNK_FLAGS);

	if (!Core.SocSdk)
		F.w_u32(m_CO_Flags.get());
	else
	{
		Flags32 tempFlags;
		tempFlags.set((u32)SocFlags::flSelected, m_RT_Flags.is(flRT_Selected));
		tempFlags.set((u32)SocFlags::flVisible, m_RT_Flags.is(flRT_Visible));
		tempFlags.set((u32)SocFlags::flLocked, false);
		tempFlags.set((u32)SocFlags::flMotion, m_CO_Flags.is(flMotion));
		tempFlags.set((u32)SocFlags::flAutoKey, m_CO_Flags.is(flAutoKey));
		tempFlags.set((u32)SocFlags::flCameraView, m_CO_Flags.is(flCameraView));
		F.w_u32(tempFlags.get());
	}

	F.close_chunk();

	F.open_chunk(CUSTOMOBJECT_CHUNK_NAME);
	F.w_stringZ(FName);
	F.close_chunk();

	F.open_chunk(CUSTOMOBJECT_CHUNK_TRANSFORM);
	F.w_fvector3(FPosition);
	F.w_fvector3(FRotation);
	F.w_fvector3(FScale);
	F.close_chunk();

	// object motion
	if (m_CO_Flags.is(flMotion))
	{
		VERIFY(m_Motion);
		F.open_chunk(CUSTOMOBJECT_CHUNK_MOTION);
		m_Motion->Save(F);
		F.close_chunk();

		F.open_chunk(CUSTOMOBJECT_CHUNK_MOTION_PARAM);
		F.w_float(m_MotionParams->t_current);
		F.close_chunk();
	}
}

void CCustomObject::OnFrame()
{
	if (m_Motion)
		AnimationOnFrame();

	if (m_RT_Flags.is(flRT_UpdateTransform))
		OnUpdateTransform();

	if (m_CO_Flags.test(flObjectInGroup) && m_pOwnerObject == NULL)
		m_CO_Flags.set(flObjectInGroup, FALSE);
}

void CCustomObject::RenderRoot(int priority, bool strictB2F)
{
	if (FParentTools->IsVisible())
		Render(priority, strictB2F);
}

void CCustomObject::Render(int priority, bool strictB2F)
{
	if ((1 == priority) && (false == strictB2F))
	{
		if (EPrefs->object_flags.is(epoDrawPivot) && Selected())
			DU_impl.DrawObjectAxis(FTransformRP, 0.1f, Selected());

		if (m_Motion && Visible() && Selected())
			AnimationDrawPath();
	}
}

bool CCustomObject::RaySelect(int flag, const Fvector &start, const Fvector &dir, bool bRayTest)
{
	float dist = UI->ZFar();

	if ((bRayTest && RayPick(dist, start, dir)) || !bRayTest)
	{
		Select(flag);
		return true;
	}

	return false;
}

bool CCustomObject::FrustumSelect(int flag, const CFrustum &frustum)
{
	if (FrustumPick(frustum))
	{
		Select(flag);
		return true;
	}

	return false;
}

bool CCustomObject::GetSummaryInfo(SSceneSummary *inf)
{
	Fbox bb;

	if (GetBox(bb))
	{
		inf->bbox.modify(bb.min);
		inf->bbox.modify(bb.max);
	}

	return true;
}

void CCustomObject::OnSynchronize()
{
	OnFrame();
}
