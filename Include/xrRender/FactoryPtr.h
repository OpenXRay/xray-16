#ifndef	FactoryPtr_included
#define FactoryPtr_included
#pragma once

#include "RenderFactory.h"
#include "../../Include/xrAPI/xrAPI.h"

#define FACTORY_PTR_INSTANCIATE(Class) \
	inline void FactoryPtr<I##Class>::CreateObject(void) \
{ \
	m_pObject = RenderFactory->Create##Class(); \
} \
	inline void FactoryPtr<I##Class>::DestroyObject(void) \
{ \
	RenderFactory->Destroy##Class(m_pObject); \
	m_pObject = NULL; \
}
	

template<class T> 
class FactoryPtr
{
public:
	FactoryPtr() { CreateObject();}
	~FactoryPtr() { DestroyObject();}

	FactoryPtr(const FactoryPtr<T> &_in)
	{
		CreateObject();
		m_pObject->Copy(*_in.m_pObject);
	}

	FactoryPtr& operator=( const FactoryPtr &_in)
	{
		m_pObject->Copy(*_in.m_pObject);
		return *this;
	}

	T& operator*() const {return *m_pObject;}
	T* operator->() const {return m_pObject;}

	// unspecified bool type
	typedef T const * (FactoryPtr::*unspecified_bool_type) () const;
	operator unspecified_bool_type () const	{return (!m_pObject ? 0 : &FactoryPtr::get);}
	bool operator!	() const { return m_pObject == 0;}

private:
	void CreateObject();
	void DestroyObject();
	T const* get() const { return m_pObject; }
private:
	T*					m_pObject;
};


#ifndef _EDITOR
	FACTORY_PTR_INSTANCIATE(UISequenceVideoItem)
	FACTORY_PTR_INSTANCIATE(UIShader)
	FACTORY_PTR_INSTANCIATE(StatGraphRender)
	FACTORY_PTR_INSTANCIATE(ConsoleRender)
	FACTORY_PTR_INSTANCIATE(RenderDeviceRender)
#	ifdef DEBUG
		FACTORY_PTR_INSTANCIATE(ObjectSpaceRender)
#	endif // DEBUG
	FACTORY_PTR_INSTANCIATE(ApplicationRender)
	FACTORY_PTR_INSTANCIATE(WallMarkArray)
	FACTORY_PTR_INSTANCIATE(StatsRender);
#endif // _EDITOR

#ifndef _EDITOR
    FACTORY_PTR_INSTANCIATE(FlareRender)
    FACTORY_PTR_INSTANCIATE(ThunderboltRender)
    FACTORY_PTR_INSTANCIATE(ThunderboltDescRender)
    FACTORY_PTR_INSTANCIATE(LensFlareRender)
    FACTORY_PTR_INSTANCIATE(RainRender)
    FACTORY_PTR_INSTANCIATE(EnvironmentRender)
    FACTORY_PTR_INSTANCIATE(EnvDescriptorRender)
    FACTORY_PTR_INSTANCIATE(EnvDescriptorMixerRender)
#endif // _EDITOR
	FACTORY_PTR_INSTANCIATE(FontRender)
/*
void FactoryPtr<IStatsRender>::CreateObject(void)
{
	m_pObject = RenderFactory->CreateStatsRender();
}

void FactoryPtr<IStatsRender>::DestroyObject(void)
{
	RenderFactory->DestroyStatsRender(m_pObject);
	m_pObject = NULL;
}
*/


#endif	//	FactoryPtr_included