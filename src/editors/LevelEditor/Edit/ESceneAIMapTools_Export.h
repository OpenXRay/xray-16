#ifndef ESceneAIMapTools_ExportH
#define ESceneAIMapTools_ExportH

//----------------------------------------------------
struct SAIParams
{
	float		fPatchSize;			// patch size
	float		fTestHeight;		// test height (center of the "tester")
	float		fCanUP;				// can reach point in up (dist)
	float		fCanDOWN;			// can reach point down  (dist)
	
	SAIParams	()
    {
		fPatchSize		= 0.7f;
		fTestHeight		= 1.0f;
		fCanUP			= 1.5f;
		fCanDOWN		= 4.0f;
	}
};

// chunks
#define E_AIMAP_VERSION  			0x0001
//----------------------------------------------------
#define E_AIMAP_CHUNK_VERSION		0x0001
#define E_AIMAP_CHUNK_BOX			0x0002
#define E_AIMAP_CHUNK_PARAMS		0x0003
#define E_AIMAP_CHUNK_NODES			0x0004
//----------------------------------------------------
#endif
 