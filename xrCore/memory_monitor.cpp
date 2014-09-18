#include "stdafx.h"

#ifdef USE_MEMORY_MONITOR
#	include <time.h>
#	include <direct.h>

#define STATIC
//#define STATIC	static

// constants
STATIC const u32				buffer_size = 512*1024;
STATIC LPCSTR					output_folder = "d:/memory_monitor_stats/";
STATIC LPCSTR					output_extension = ".bin";

// for internal use only
STATIC bool						detaching = false;
STATIC CRITICAL_SECTION			critical_section;

namespace memory_monitor {

LPCSTR inline file_name()
{
	static string_path			file = " ";
	if(file[0]==' '){
		_mkdir					(output_folder);

		__time64_t				long_time;
		_time64					(&long_time);
		tm						new_time;
		new_time				= *_localtime64(&long_time);
		string256				file_name;
		strftime				(file_name,sizeof(file_name),"%Y.%m.%d.%H.%M.%S",&new_time);
		strconcat				(sizeof(file),file,output_folder,file_name,output_extension);
	}
	return file;
}

STATIC inline FILE *file()
{
	static FILE					*m_file = 0;
	static char					buffer[buffer_size];
	if (!m_file) {
		m_file					= fopen(file_name(),"wb");
		VERIFY					(m_file);
		setvbuf					(m_file,buffer,_IOFBF,buffer_size);
	}

	return						(m_file);
}

union _allocation_size {
	struct {
		u32	allocation			: 1;
		u32 size				: 31;
	};
	u32		allocation_size;
};

STATIC bool use_monitor					()
{
	return						(!!strstr(GetCommandLine(),"-memory_monitor"));
}
}

void memory_monitor::flush_each_time	(const bool &value)
{
	if (!use_monitor())
		return;

	detaching					= value;
	if (detaching)
		fflush					(file());
}

namespace memory_monitor {
STATIC void initialize					()
{
	VERIFY						(use_monitor());
	InitializeCriticalSection	(&critical_section);
}
}
XRCORE_API int memory_monitor::counter = 0;
XRCORE_API int memory_monitor::counter_alloc = 0;
XRCORE_API int memory_monitor::counter_free = 0;
void memory_monitor::monitor_alloc		(const void *allocation_address, const u32 &allocation_size, LPCSTR allocation_description)
{
	counter++;
	counter_alloc++;
	//if(0==stricmp(allocation_description,"char"))
	//{
	//	int d = 65+56;
	//}
	if (!use_monitor())
		return;
	static bool initialized		= false;
	if (!initialized) {
		initialized				= true;
		initialize				();
	}

//	if (!detaching)
		EnterCriticalSection	(&critical_section);	

	_allocation_size			temp;
	temp.allocation				= 1;
	temp.size					= allocation_size;
	fwrite						(&temp,sizeof(temp),1,file());
	fwrite						(&allocation_address,sizeof(allocation_address),1,file());
	fwrite						(allocation_description,(xr_strlen(allocation_description) + 1)*sizeof(char),1,file());

	if (!detaching)
		;
	else
		fflush					(file());

	LeaveCriticalSection		(&critical_section);
}

void memory_monitor::monitor_free		(const void *deallocation_address)
{
	counter++;
	counter_free++;
	if (!use_monitor())
		return;

//	if (!detaching)
		EnterCriticalSection	(&critical_section);

	if (deallocation_address) {
		_allocation_size		temp;
		temp.allocation_size	= 0;
		fwrite					(&temp,sizeof(temp),1,file());
		fwrite					(&deallocation_address,sizeof(deallocation_address),1,file());
	}

	if (!detaching)
		;
	else
		fflush					(file());

	LeaveCriticalSection		(&critical_section);
}

void memory_monitor::make_checkpoint	(LPCSTR checkpoint_name)
{
	fflush					(file());
	string_path				fn;
	strconcat				(sizeof(fn),fn,output_folder,checkpoint_name,output_extension);

	Msg						("Creating memory checkpoint file[%s]...", fn);
	CopyFile				(file_name(),fn,FALSE);
	Msg						("Done");
}

#endif // USE_MEMORY_MONITOR