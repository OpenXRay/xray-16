#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"
#include "IGame_ObjectPool.h"
#include "xr_object.h"
#include "xrServerEntities/smart_cast.h"

IGame_ObjectPool::IGame_ObjectPool(void) {}
IGame_ObjectPool::~IGame_ObjectPool(void) { R_ASSERT(m_PrefetchObjects.empty()); }
void IGame_ObjectPool::prefetch()
{
    R_ASSERT(m_PrefetchObjects.empty());

    int p_count = 0;
    GEnv.Render->model_Logging(false);

    string256 section;
    // prefetch objects
    strconcat(sizeof(section), section, "prefetch_objects_", g_pGamePersistent->m_game_params.m_game_type);
    CInifile::Sect const& sect = pSettings->r_section(section);
    for (const auto& item : sect.Data)
    {
        CLASS_ID CLS = pSettings->r_clsid(item.first.c_str(), "class");
        p_count++;
        IGameObject* pObject = smart_cast<IGameObject*>(NEW_INSTANCE(CLS));
        pObject->Load(item.first.c_str());
        VERIFY2(pObject->cNameSect().c_str(), item.first.c_str());
        m_PrefetchObjects.push_back(pObject);
    }

    // out statistic
    GEnv.Render->model_Logging(true);
}

void IGame_ObjectPool::clear()
{
    // Clear POOL
    for (auto& it : m_PrefetchObjects)
        xr_delete(it);
    m_PrefetchObjects.clear();
}

IGameObject* IGame_ObjectPool::create(pcstr name)
{
    CLASS_ID CLS = pSettings->r_clsid(name, "class");
    IGameObject* O = smart_cast<IGameObject*>(NEW_INSTANCE(CLS));
    O->cNameSect_set(name);
    O->Load(name);
    O->PostLoad(name); //--#SM+#--
    return O;
}

void IGame_ObjectPool::destroy(IGameObject* O) { xr_delete(O); }
/*
void IGame_ObjectPool::prefetch ()
{
R_ASSERT (map_POOL.empty());

u32 mem_0 = Memory.mem_usage();
float p_time = 1000.f*Device.GetTimerGlobal()->GetElapsed_sec();
int p_count = 0;
GlobalEnv.Render->model_Logging (false);

string256 section;
// prefetch objects
strconcat (section,"prefetch_objects_",g_pGamePersistent->m_game_params.m_game_type);
CInifile::Sect& sect = pSettings->r_section(section);
for (CInifile::SectIt I=sect.begin(); I!=sect.end(); I++) {
CInifile::Item& item= *I;
CLASS_ID CLS = pSettings->r_clsid(item.first.c_str(),"class");
int count = atoi(item.second.c_str());
count += (count==0)?1:0;
R_ASSERT2 ((count>0) && (count<=128), "Too many objects for prefetching");
p_count += count;
for (int c=0; c<count; c++){
IGameObject* pObject= (IGameObject*) NEW_INSTANCE(CLS);
pObject->Load (item.first.c_str());
VERIFY2 (pObject->cNameSect().c_str(),item.first.c_str());
map_POOL.insert (std::make_pair(pObject->cNameSect(),pObject));
}
}

// out statistic
GlobalEnv.Render->model_Logging (true);
p_time = 1000.f*Device.GetTimerGlobal()->GetElapsed_sec() - p_time;
u32 p_mem = Memory.mem_usage() - mem_0;
if (p_count){
float a_time = float(p_time)/float(p_count);
Msg ("* [Object-prefetch] objects: %d", p_count);
Msg ("* [Object-prefetch] time: %d ms", iFloor(p_time));
Msg ("* [Object-prefetch] memory: %dKb", p_mem/1024);
Msg ("* [Object-prefetch] average: %2.2f ms, %d bytes", a_time, p_mem/p_count);
}
}

void IGame_ObjectPool::clear ( )
{
// Clear POOL
for (POOL_IT it=map_POOL.begin(); it!=map_POOL.end(); it++)
xr_delete (it->second);
map_POOL.clear();
}

IGameObject* IGame_ObjectPool::create ( pcstr name )
{
string256 l_name;
POOL_IT it = map_POOL.find (shared_str(xr_strlwr(xr_strcpy(l_name,name))));
if (it!=map_POOL.end())
{
// Instance found
IGameObject* O = it->second;
map_POOL.erase (it);
return O;
} else {
// Create and load _new instance
// Msg ("! Uncached loading '%s'...",name);
CLASS_ID CLS = pSettings->r_clsid (name,"class");
IGameObject* O = (IGameObject*) NEW_INSTANCE (CLS);
O->Load (name);
return O;
}
}

void IGame_ObjectPool::destroy ( IGameObject* O )
{
map_POOL.insert (std::make_pair(O->cNameSect(),O));
}
*/
