#ifndef logH
#define logH

#define VPUSH(a)	((a).x), ((a).y), ((a).z)

void 	XRCORE_API	__cdecl		Msg	(LPCSTR format, ...);
void 	XRCORE_API		Log			(LPCSTR msg);
void 	XRCORE_API		Log			(LPCSTR msg);
void 	XRCORE_API		Log			(LPCSTR msg, LPCSTR			dop);
void 	XRCORE_API		Log			(LPCSTR msg, u32			dop);
void 	XRCORE_API		Log			(LPCSTR msg, int  			dop);
void 	XRCORE_API		Log			(LPCSTR msg, float			dop);
void 	XRCORE_API		Log			(LPCSTR msg, const Fvector& dop);
void 	XRCORE_API		Log			(LPCSTR msg, const Fmatrix& dop);
void 	XRCORE_API		LogWinErr	(LPCSTR msg, long 			err_code);

typedef void	( * LogCallback)	(LPCSTR string);
LogCallback	XRCORE_API			SetLogCB	(LogCallback cb);
void 	XRCORE_API				CreateLog	(BOOL no_log=FALSE);
void 							InitLog		();
void 							CloseLog	();
void	XRCORE_API				FlushLog	();

extern 	XRCORE_API	xr_vector<shared_str>*		LogFile;
extern 	XRCORE_API	BOOL						LogExecCB;

#endif

