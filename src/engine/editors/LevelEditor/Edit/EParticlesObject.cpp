//----------------------------------------------------
// file: EParticlesObject.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "EParticlesObject.h"
#include "../ECORE/Editor/D3DUtils.h"
#include "..\..\Layers\xrRender\PSLibrary.h"
#include "../ECore/Editor/ui_main.h"

#define CPSOBJECT_VERSION  				0x0013
//----------------------------------------------------
#define CPSOBJECT_CHUNK_VERSION			0x0001
#define CPSOBJECT_CHUNK_REFERENCE		0x0002
#define CPSOBJECT_CHUNK_PARAMS			0x0003
#define CPSOBJECT_CHUNK_GAMETYPE		0x0004
//----------------------------------------------------

#define PSOBJECT_SIZE 0.5f

//using namespace PS;

EParticlesObject::EParticlesObject(LPVOID data, LPCSTR name):CCustomObject(data,name)
{
	Construct(data);
}
//----------------------------------------------------

void EParticlesObject::Construct(LPVOID data)
{
	ClassID   	= OBJCLASS_PS;
    m_Particles	= 0;
    m_GameType.SetDefaults();
}
//----------------------------------------------------

EParticlesObject::~EParticlesObject()
{
	::Render->model_Delete	(dynamic_cast<IRenderVisual*>(m_Particles) );
}
//----------------------------------------------------

bool EParticlesObject::GetBox( Fbox& box ) const
{
	box.set( PPosition, PPosition );
	box.min.x -= PSOBJECT_SIZE;
	box.min.y -= PSOBJECT_SIZE;
	box.min.z -= PSOBJECT_SIZE;
	box.max.x += PSOBJECT_SIZE;
	box.max.y += PSOBJECT_SIZE;
	box.max.z += PSOBJECT_SIZE;
    return true;
}
//----------------------------------------------------

void EParticlesObject::OnUpdateTransform()
{
	inherited::OnUpdateTransform	();
	Fvector v={0.f,0.f,0.f};
	if (m_Particles) m_Particles->UpdateParent(_Transform(),v,FALSE);
}
//----------------------------------------------------

void EParticlesObject::OnFrame()
{
	inherited::OnFrame();
    Fbox bb; GetBox(bb);
    if (::Render->occ_visible(bb))
	    if (m_Particles)
        	m_Particles->OnFrame(EDevice.dwTimeDelta);
}
//----------------------------------------------------

void EParticlesObject::Render(int priority, bool strictB2F)
{
	inherited::Render(priority,strictB2F);
    Fbox bb; GetBox(bb);
	if (::Render->occ_visible(bb)){
        RCache.set_xform_world(Fidentity);
	    if (1==priority){
            if (false==strictB2F){
                // draw emitter
	    		EDevice.SetShader(EDevice.m_WireShader);
                if( !Selected() )
    				DU_impl.DrawCross	(PPosition,0.30f,0.1f,0.3f,0.3f,0.3f,0.3f,0xFFFFEBAA,false);
                    
				Fvector p = PPosition;
                DU_impl.DrawRomboid	(p,0.1f,0x0AFFEBAA);
                if( Selected() )
                {
                    Fbox bb; GetBox(bb);
                    u32 clr = 0xFFFFFFFF;
                    DU_impl.DrawSelectionBoxB(bb,&clr);
                }
            }
        }
        if (m_Particles)
        	::Render->model_Render(dynamic_cast<IRenderVisual*>(m_Particles), _Transform(),priority,strictB2F,1.f);
    }
}
//----------------------------------------------------

void EParticlesObject::RenderSingle()
{
	Render(1,false);
	Render(1,true);
}
//----------------------------------------------------

bool EParticlesObject::FrustumPick(const CFrustum& frustum)
{
    return (frustum.testSphere_dirty(PPosition,PSOBJECT_SIZE))?true:false;
}
//----------------------------------------------------

bool EParticlesObject::RayPick(float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf)
{
	Fvector pos,ray2;
    pos.set(PPosition);
	ray2.sub( pos, start );

    float d = ray2.dotproduct(direction);
    if( d > 0  ){
        float d2 = ray2.magnitude();
        if( ((d2*d2-d*d) < (PSOBJECT_SIZE*PSOBJECT_SIZE)) && (d>PSOBJECT_SIZE) ){
        	if (d<distance){
	            distance = d;
    	        return true;
            }
        }
    }
	return false;
}
//----------------------------------------------------

void EParticlesObject::Play()
{
	if (m_Particles) m_Particles->Play();
}
//----------------------------------------------------

void EParticlesObject::Stop()
{
	if (m_Particles) m_Particles->Stop();
}
//----------------------------------------------------
bool EParticlesObject::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	u32 version = ini.r_u32(sect_name,"version");


	inherited::LoadLTX(ini, sect_name);

    if(version>=0x0012)
		m_GameType.LoadLTX(ini, sect_name, false);

    m_RefName		= ini.r_string(sect_name, "ref_name");
    if (!Compile(*m_RefName))
    {
        ELog.DlgMsg( mtError, "EParticlesObject: '%s' not found in library", *m_RefName );
        return false;
    }
    return true;
}

void EParticlesObject::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	CCustomObject::SaveLTX(ini, sect_name);

	ini.w_u32			(sect_name, "version", CPSOBJECT_VERSION);

    ini.w_string		(sect_name, "ref_name", m_RefName.c_str());
	m_GameType.SaveLTX	(ini, sect_name);
}

bool EParticlesObject::LoadStream(IReader& F)
{
	u16 version = 0;

    R_ASSERT(F.r_chunk(CPSOBJECT_CHUNK_VERSION,&version));
    if(version<0x0011)
    {
        ELog.DlgMsg( mtError, "PSObject: Unsupported version.");
        return false;
    }

	inherited::LoadStream(F);


    R_ASSERT(F.find_chunk(CPSOBJECT_CHUNK_REFERENCE));

    F.r_stringZ(m_RefName);

    if(version>=0x0013)
    {
    	R_ASSERT(F.find_chunk(CPSOBJECT_CHUNK_GAMETYPE));
		m_GameType.LoadStream(F);
    }

    if (!Compile(*m_RefName))
    {
        ELog.DlgMsg( mtError, "EParticlesObject: '%s' not found in library", *m_RefName );
        return false;
    }

    return true;
}
//----------------------------------------------------

void EParticlesObject::SaveStream(IWriter& F)                               
{
	inherited::SaveStream(F);

	F.open_chunk	(CPSOBJECT_CHUNK_VERSION);
	F.w_u16			(CPSOBJECT_VERSION);
	F.close_chunk	();

	F.open_chunk	(CPSOBJECT_CHUNK_REFERENCE);
    F.w_stringZ		(m_RefName);
	F.close_chunk	();

	F.open_chunk	(CPSOBJECT_CHUNK_GAMETYPE);
	m_GameType.SaveStream(F);
	F.close_chunk	();
}
//----------------------------------------------------

void EParticlesObject::OnDeviceCreate()
{
}

void EParticlesObject::OnDeviceDestroy()
{
}

bool EParticlesObject::ExportGame(SExportStreams* F)
{
	SExportStreamItem& I	= F->pe_static;

    if(I.chunk==0)
    {
        I.stream.open_chunk		(I.chunk++);
        I.stream.w_u32			(1); //version
        I.stream.close_chunk	();
    }
    
	I.stream.open_chunk		(I.chunk++);
    I.stream.w_u16			(m_GameType.m_GameType.get());
    I.stream.w_stringZ		(m_RefName);
    I.stream.w				(&_Transform(),sizeof(Fmatrix));
    I.stream.close_chunk	();
    return true;
}
//----------------------------------------------------

bool EParticlesObject::Compile(LPCSTR ref_name)
{
	::Render->model_Delete	(dynamic_cast<IRenderVisual*>(m_Particles) );
    if (ref_name)
    {
    	IRenderVisual* base = ::Render->model_CreateParticles(ref_name);
		m_Particles 		= dynamic_cast<IParticleCustom*>(base);
        if (m_Particles){
			UpdateTransform	();
		    m_RefName		= ref_name;
            return true;
        }
    }
    m_RefName				= "";
    return false;
}

void EParticlesObject::OnRefChange(PropValue* V)
{
	if (!Compile(*m_RefName)){
        ELog.Msg( mtError, "Can't compile particle system '%s'", *m_RefName );
    }else{
    	ExecCommand(COMMAND_REFRESH_PROPERTIES);
    }
}

void EParticlesObject::OnControlClick(ButtonValue* sender, bool& bModif, bool& bSafe)
{
	ButtonValue* V = dynamic_cast<ButtonValue*>(sender); R_ASSERT(V);
    switch (V->btn_num){
    case 0: Play();	break;
    case 1: Stop();	break;
	}
    UI->RedrawScene();
    bModif = false;
}

void EParticlesObject::FillProp(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProp(pref, items);
    PropValue* V;
    V=PHelper().CreateChoose	(items,PrepareKey(pref, "Reference"),&m_RefName, smParticles);
    V->OnChangeEvent.bind		(this,&EParticlesObject::OnRefChange);
	ButtonValue* B;
    B=PHelper().CreateButton	(items,PrepareKey(pref,"Controls"), 	"Play,Stop",0);
    B->OnBtnClickEvent.bind		(this,&EParticlesObject::OnControlClick);

	m_GameType.FillProp			(pref, items);
}
//----------------------------------------------------

bool EParticlesObject::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
/*
	if (!m_RefName.IsEmpty()&&m_PE.GetDefinition()){ 
    	inf->pe_static.insert(m_PE.GetDefinition()->m_Name);
		if (m_PE.GetDefinition()->m_TextureName&&m_PE.GetDefinition()->m_TextureName[0]) inf->textures.insert(ChangeFileExt(m_PE.GetDefinition()->m_TextureName,"").LowerCase());
    }
	inf->pe_static_cnt++;
*/
	return true;
}
//----------------------------------------------------

