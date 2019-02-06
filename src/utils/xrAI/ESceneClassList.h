#ifndef ESceneClassListH
#define ESceneClassListH

// refs
class CCustomObject;

typedef u32 ObjClassID;

//----------------------------------------------------
enum
{
    OBJCLASS_DUMMY = -1,
    OBJCLASS_FIRST_CLASS = 0,
    OBJCLASS_GROUP = 0,
    OBJCLASS_GLOW = 1,
    OBJCLASS_SCENEOBJECT = 2,
    OBJCLASS_LIGHT = 3,
    OBJCLASS_SHAPE = 4,
    OBJCLASS_SOUND_SRC = 5,
    OBJCLASS_SPAWNPOINT = 6,
    OBJCLASS_WAY = 7,
    OBJCLASS_SECTOR = 8,
    OBJCLASS_PORTAL = 9,
    OBJCLASS_SOUND_ENV = 10,
    OBJCLASS_PS = 11,
    OBJCLASS_DO = 12,
    OBJCLASS_AIMAP = 13,
    OBJCLASS_WM = 14,
    OBJCLASS_FOG_VOL = 15,
    OBJCLASS_COUNT = 16,
    OBJCLASS_force_dword = u32(-1)
};

//----------------------------------------------------

typedef xr_list<CCustomObject*> ObjectList;
typedef ObjectList::iterator ObjectIt;
typedef xr_map<ObjClassID, ObjectList> ObjectMap;
typedef ObjectMap::iterator ObjectPairIt;

#endif
