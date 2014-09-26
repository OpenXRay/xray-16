#ifndef _BACKUP_H_
#define _BACKUP_H_

void BackupInit(void);

BOOL BackupExists(void);

BOOL WriteToBackup(const void * src, int len);
BOOL ReadFromBackup(void * dst, int len);

#endif