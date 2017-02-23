#include "..\nonport.h"
#include "backup.h"
#include "screen.h"

//#define SUPPORT_FLASH
//#define SUPPORT_FRAM

static const CARDBackupType	BackupTypes[] =
{
	CARD_BACKUP_TYPE_EEPROM_4KBITS,
	CARD_BACKUP_TYPE_EEPROM_64KBITS,
	CARD_BACKUP_TYPE_EEPROM_512KBITS,
#if defined(SUPPORT_FLASH)
	CARD_BACKUP_TYPE_FLASH_2MBITS,
#endif
#if defined(SUPPORT_FRAM)
	CARD_BACKUP_TYPE_FRAM_256KBITS,
#endif
};
static const char * BackupTypeDescriptions[] =
{
	"EEPROM 4 kb",
	"EEPROM 64 kb",
	"EEPROM 512 kb",
#if defined(SUPPORT_FLASH)
	"FLASH 2 mb",
#endif
#if defined(SUPPORT_FRAM)
	"FRAM 256 kb"
#endif
};
static const int NumBackupTypes = (sizeof(BackupTypes) / sizeof(BackupTypes[0]));
static const CARDBackupType NullBackupType = CARD_BACKUP_TYPE_NOT_USE;

static CARDBackupType InstalledBackupType;
static int InstalledBackupTypeIndex;

static u16 LockID;

static BOOL TestForBackupType(int index)
{
	const CARDBackupType type = BackupTypes[index];
	u32 totalSize;
	BOOL result;
	u8 buffer;

	Printf("Testing for %s\n", BackupTypeDescriptions[index]);

	CARD_LockBackup(LockID);
	{
		CARD_IdentifyBackup(type);

		totalSize = CARD_GetBackupTotalSize();

		// set the buffer
		buffer = (u8)(rand() & 0xFF);

		// write the buffer to the card
		if(CARD_IsBackupEeprom())
			result = CARD_WriteAndVerifyEeprom(totalSize - 1, &buffer, 1);
		else if(CARD_IsBackupFlash())
			result = CARD_WriteAndVerifyFlash(totalSize - 1, &buffer, 1);
		else if(CARD_IsBackupFram())
			result = CARD_WriteAndVerifyFram(totalSize - 1, &buffer, 1);
	}
	CARD_UnlockBackup(LockID);

	return result;
}

static void DetermineBackupType(void)
{
	int i;

	// determine what, if any, backup type is installed
	for(i = 0 ; i < NumBackupTypes ; i++)
	{
		if(TestForBackupType(i) == TRUE)
		{
			InstalledBackupType = BackupTypes[i];
			InstalledBackupTypeIndex = i;
			return;
		}
	}

	InstalledBackupType = NullBackupType;
	InstalledBackupTypeIndex = -1;
}

void BackupInit(void)
{
	// get an id for locking the card
	s32 lockID = OS_GetLockID();
	if(lockID == OS_LOCK_ID_ERROR)
		OS_Panic("OS_GetLockID() failed\n");
	LockID = (u16)lockID;

	// figure out the backup type
	DetermineBackupType();

	// make sure we have the right type identified
	if(BackupExists())
	{
		CARD_LockBackup(LockID);
		{
			CARD_IdentifyBackup(InstalledBackupType);
		}
		CARD_UnlockBackup(LockID);
	}

	// show what we detected
	if(BackupExists())
		Printf("Backup type: %s\n", BackupTypeDescriptions[InstalledBackupTypeIndex]);
	else
		Printf("No backup card detected\n");
}

BOOL BackupExists(void)
{
	return (InstalledBackupType != NullBackupType)?TRUE:FALSE;
}

static const int StartPos = 32;

BOOL WriteToBackup(const void * src, int len)
{
	BOOL result;

	if(!BackupExists())
		return FALSE;

	CARD_LockBackup(LockID);

	if(CARD_IsBackupEeprom())
		result = CARD_WriteAndVerifyEeprom(StartPos, src, (u32)len);
	else if(CARD_IsBackupFlash())
		result = CARD_WriteAndVerifyFlash(StartPos, src, (u32)len);
	else if(CARD_IsBackupFram())
		result = CARD_WriteAndVerifyFram(StartPos, src, (u32)len);
	else
		result = FALSE;

	CARD_UnlockBackup(LockID);

	if(result == FALSE)
	{
		u32 code = CARD_GetResultCode();
		Printf("CARD-WRITE-FAILURE: %d\n", code);
		while(1)
			;
	}

	return result;
}

BOOL ReadFromBackup(void * dst, int len)
{
	BOOL result;

	if(!BackupExists())
		return FALSE;

	CARD_LockBackup(LockID);

	if(CARD_IsBackupEeprom())
		result = CARD_ReadEeprom(StartPos, dst, (u32)len);
	else if(CARD_IsBackupFlash())
		result = CARD_ReadFlash(StartPos, dst, (u32)len);
	else if(CARD_IsBackupFram())
		result = CARD_ReadFram(StartPos, dst, (u32)len);
	else
		result = FALSE;

	CARD_UnlockBackup(LockID);

	if(result == FALSE)
	{
		u32 code = CARD_GetResultCode();
		Printf("CARD-READ-FAILURE: %d\n", code);
		while(1)
			;
	}

	return result;
}