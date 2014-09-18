//---------------------------------------------------------------------------
#ifndef SoundManagerH
#define SoundManagerH

#include "../xrEProps/folderlib.h"

// refs
class ESoundThumbnail;

class ECORE_API CSoundManager
{
public:
	void 		MakeGameSound		(ESoundThumbnail* THM, LPCSTR src_name, LPCSTR game_name);
				CSoundManager		(){}
	virtual		~CSoundManager		(){;}
                    
    BOOL __stdcall RemoveSound		(LPCSTR fname, EItemType type);
	void __stdcall RenameSound		(LPCSTR p0, LPCSTR p1, EItemType type);

	// texture routines
    int			GetSounds			(FS_FileSet& files, BOOL bFolders=FALSE);
    int			GetGameSounds		(FS_FileSet& files);
    int			GetSoundEnvs		(AStringVec& items);

	int 		GetLocalNewSounds	(FS_FileSet& files);
//	void		SafeCopyLocalToServer(FS_FileSet& files);
	void		SynchronizeSounds	(bool sync_thm, bool sync_game, bool bForceGame, FS_FileSet* source_map, AStringVec* sync_list_without_extention, FS_FileSet* modif_map=0);
//	void 		ChangeFileAgeTo		(FS_FileSet* tgt_map, int age);
    void		CreateSoundThumbnail(ESoundThumbnail* THM, const AnsiString& src_name, LPCSTR path=0, bool bSetDefParam=true);
	void		CleanupSounds		();

    bool		OnCreate			();
    void		OnDestroy			();

    virtual void OnFrame			();

    virtual bool Validate			(){return true;}

    void		MuteSounds			(BOOL bVal);

    void 		RefreshSounds		(bool bSync);

	xr_string	UpdateFileName		(xr_string& fn);
};

extern ECORE_API CSoundManager* SndLib;
//---------------------------------------------------------------------------
#endif
