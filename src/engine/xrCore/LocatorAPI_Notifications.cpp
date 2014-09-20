// LocatorAPI.cpp: implementation of the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "LocatorAPI_Notifications.h"

void CThread::startup(void* P)
{
	CThread* T 			= (CThread*)P;
	T->Execute			();
}

static CRITICAL_SECTION CS;
//---------------------------------------------------------------------------
// TShellChangeThread -------------------------------------------------------
CFS_PathNotificator::CFS_PathNotificator():CThread(0)
{
    FMutex 				= CreateMutex(NULL, true /* initial owner - must be Release'd by this thread*/, NULL);
    if (FMutex)
        WaitForSingleObject(FMutex, INFINITE);
}
//---------------------------------------------------------------------------

CFS_PathNotificator::~CFS_PathNotificator(void)
{
	for (PathIt it=events.begin(); it!=events.end(); it++){
    	Path& P			= *it;
    	P.FChangeEvent	= 0;
        if (P.FWaitHandle != INVALID_HANDLE_VALUE){
            HANDLE hOld 	= P.FWaitHandle;
            P.FWaitHandle= 0;
            FindCloseChangeNotification(hOld);
        }
    }
	events.clear		();
}
//---------------------------------------------------------------------------

void CFS_PathNotificator::RegisterPath(FS_Path& path)
{                     
//	R_ASSERT2(Suspended,"Can't register path. Thread already started.");
	shared_str dir			= path.m_Path;
	for (PathIt it=events.begin(); it!=events.end(); it++)
    	if ((it->FDirectory==dir)&&(it->bRecurse==path.m_Flags.is(FS_Path::flRecurse))) return;

    events.push_back	(Path());
    Path& P				= events.back();
    P.FDirectory		= path.m_Path;
    P.bRecurse			= path.m_Flags.is(FS_Path::flRecurse);
    P.FChangeEvent.bind	(&path,&FS_Path::rescan_path_cb);
    P.FWaitHandle 		= INVALID_HANDLE_VALUE;
}
//---------------------------------------------------------------------------

void CFS_PathNotificator::Execute(void)
{
    EnterCriticalSection(&CS);
	for (PathIt it=events.begin(); it!=events.end(); it++){
    	Path& P			= *it;
        P.FWaitHandle	= FindFirstChangeNotification(P.FDirectory.c_str(), P.bRecurse, FNotifyOptionFlags);
        if (P.FWaitHandle == INVALID_HANDLE_VALUE)
#ifndef __BORLANDC__
            Debug.fatal	(DEBUG_INFO,"Can't create notify handle for path: '%s'\nwith error: '%s'",P.FDirectory.c_str(),Debug.error2string(GetLastError()));
#else // __BORLANDC__
            Debug.fatal	("Can't create notify handle for path: '%s'\nwith error: '%s'",P.FDirectory.c_str(),Debug.error2string(GetLastError()));
#endif // __BORLANDC__
    }
    LeaveCriticalSection(&CS);
//	if (FWaitHandle == INVALID_HANDLE_VALUE)
//		return;
    while(!Terminated)
    {
        HANDLEVec hHandles;
        hHandles.push_back(FMutex);
		for (PathIt it=events.begin(); it!=events.end(); it++)
        	hHandles.push_back(it->FWaitHandle);

        int Obj = WaitForMultipleObjects(hHandles.size(), &*hHandles.begin(), false, INFINITE);
        if (Obj==WAIT_OBJECT_0){
            ReleaseMutex(FMutex);
            break;
        }else if (Obj>WAIT_OBJECT_0){
        	u32 idx		= Obj-WAIT_OBJECT_0-1;
            if (idx<events.size()){
	            Path& P		= events[idx];
    	        if (!P.FChangeEvent.empty())P.FChangeEvent				();
        	    if (P.FWaitHandle) 			FindNextChangeNotification	(P.FWaitHandle);
            }
        }else 
        	return;
    }
}
//---------------------------------------------------------------------------

void CLocatorAPI::SetEventNotification()
{
	InitializeCriticalSection	(&CS);
    FThread 					= xr_new<CFS_PathNotificator>();
	FThread->FNotifyOptionFlags	= FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE;
	for(PathPairIt p_it=pathes.begin(); p_it!=pathes.end(); p_it++)
    	if (p_it->second->m_Flags.is(FS_Path::flNotif)) FThread->RegisterPath(*p_it->second);
    FThread->Start				();
}
 
void CLocatorAPI::ClearEventNotification()
{
    if (FThread){
    	FThread->Terminate		();
	    ReleaseMutex			(FThread->FMutex); // this current thread must release the mutex
	    xr_delete				(FThread);
    }
	DeleteCriticalSection		(&CS);
}

