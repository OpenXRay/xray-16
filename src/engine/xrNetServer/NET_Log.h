#pragma once

#include "net_shared.h"
struct SLogPacket
{
	u32			m_u32Time;
	u32			m_u32Size;
	u16			m_u16Type;
	string64	m_sTypeStr;
	bool		m_bIsIn;
};
class INetLog
{
private:
	FILE*		m_pLogFile;
	string1024  m_cFileName;
	u32			m_dwStartTime;

	xrCriticalSection		m_cs;
	
	xr_vector<SLogPacket>	m_aLogPackets;

	void		FlushLog();
	
public:
	INetLog(LPCSTR sFileName, u32 dwStartTime);
	~INetLog();	

	void		LogPacket(u32 Time, NET_Packet* pPacket, bool IsIn = FALSE);
	void		LogData(u32 Time, void* data, u32 size, bool IsIn = FALSE);
};

/*
// Singleton template definition 
template <class T> class CSingleton {
private:
	static T*	_self;
	static int	_refcount;
public:	
	//whether singleton will delete itself on FreeInst
	//when _refcount = 0
	//otherwise user should call DestroySingleton() manually
	static bool _on_self_delete;
public:
	CSingleton			()	{}
	virtual			~CSingleton			()	{_self=NULL;}

	static			void DestroySingleton	()	{
		if(!_self) return;
		Log			("DestroySingleton::RefCounter:",_refcount);
		VERIFY(_on_self_delete == false); 
		VERIFY(_refcount == 0);
		xr_delete(_self);
	};
public:
	static	T*		Instance	() {
		if(!_self) _self=xr_new<T>(); 
		++_refcount;
		return _self;
	}
	void			FreeInst	() {
		if(0 == --_refcount) {
			if(_on_self_delete){
				CSingleton<T> *ptr = this;
				xr_delete(ptr);
			}
		} 
	}
};

template <class T> T*	CSingleton<T>::_self			= NULL;
template <class T> int	CSingleton<T>::_refcount		= 0;
template <class T> bool CSingleton<T>::_on_self_delete	= true;
*/