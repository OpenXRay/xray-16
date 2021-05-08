#pragma once

#define RPOINT_CHOOSE_NAME "$rpoint"
#define ENVMOD_CHOOSE_NAME "$env_mod"

enum EPointType
{
    ptRPoint = 0,
    ptEnvMod,
    ptSpawnPoint,
    ptMaxType,
    pt_force_dword = u32(-1)
};

enum EWayType
{
    wtPatrolPath = 0,
    wtMaxType,
    wt_force_dword = u32(-1)
};

enum ERPpointType
{ // [0..255]
    rptActorSpawn = 0,
    rptArtefactSpawn,
    rptItemSpawn,
    rptLast = 0xff
};

enum EEnvModUsedParams
{
    eViewDist = (1 << 0),
    eFogColor = (1 << 1),
    eFogDensity = (1 << 2),
    eAmbientColor = (1 << 3),
    eSkyColor = (1 << 4),
    eHemiColor = (1 << 5)
};

/* // XXX: find better place for this (need cpp)
xr_token rpoint_type[] = {
    {"Actor Spawn", rptActorSpawn},
    {"Artefact Spawn", rptArtefactSpawn},
    {"Item Spawn", rptItemSpawn},
    {nullptr, rptLast}
};
*/

// BASE offset
#define WAY_BASE 0x1000
#define POINT_BASE 0x2000

// POINT chunks
#define RPOINT_CHUNK (POINT_BASE + ptRPoint)

// WAY chunks
#define WAY_PATROLPATH_CHUNK (WAY_BASE + wtPatrolPath)
//----------------------------------------------------

#define WAYOBJECT_VERSION 0x0013
//----------------------------------------------------
#define WAYOBJECT_CHUNK_VERSION 0x0001
#define WAYOBJECT_CHUNK_POINTS 0x0002
#define WAYOBJECT_CHUNK_LINKS 0x0003
#define WAYOBJECT_CHUNK_TYPE 0x0004
#define WAYOBJECT_CHUNK_NAME 0x0005

/*
- chunk RPOINT_CHUNK
    - chunk #0
        vector3	(PPosition);
        vector3	(PRotation);
        u8		(team_id);
        u8		(type)
        u16		(reserved)
    ...
    - chunk #n

- chunk WAY_PATH_CHUNK
    - chunk #0
        chunk WAYOBJECT_CHUNK_VERSION
            word (version)
        chunk WAYOBJECT_CHUNK_NAME
            stringZ (Name)
        chunk WAY_CHUNK_TYPE
            dword EWayType (type)
        chunk WAY_CHUNK_POINTS
            word (count)
            for (i=0; i<count; ++i){
                Fvector (pos)
                dword	(flags)
                stringZ	(name)
            }
        chunk WAY_CHUNK_LINKS
            word (count)
            for (i=0; i<count; ++i){
                word 	(from)
                word 	(to)
                float	(probability)
            }
    ...
    - chunk #n
- chunk WAY_JUMP_CHUNK
    -//-
- chunk WAY_TRAFFIC_CHUNK
    -//-
- chunk WAY_CUSTOM_CHUNK
    -//-

*/
