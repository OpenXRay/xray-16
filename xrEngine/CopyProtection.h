#pragma once

#ifndef DEBUG
//#define USE_COPYPROTECTION
#endif

// [4/28/2004]
#ifdef	USE_COPYPROTECTION

#pragma pack(push,1)
typedef struct SECUROM_PID{
	int pid_error;
	unsigned long pid_version;
	union{
		unsigned char pid_buf[264];
		struct{
			unsigned char pid_reserved[3];
			unsigned char pid_type;
			unsigned long pid_length;
			unsigned char pid_data[256];
		};
	};
}tSECUROM_PID;
#pragma pack(pop)

IC	void	CheckCopyProtection	()
{
	u32		ret;

	__asm{
		mov eax,8791h;
		mov ebx,2107h;
		mov ecx,6345h;
	};
	ret=GetVersion();

	if(ret!=0x6345){
		MessageBox(NULL,"Copy protection violation!","Error",MB_OK | MB_ICONHAND);
		TerminateProcess(GetCurrentProcess(),0x66);
	}
}
#else
IC	void	CheckCopyProtection	()	{
}
#endif
