#pragma once

#include "..\..\Include\xrRender\ThunderboltDescRender.h"

class IRender_DetailModel;

class glThunderboltDescRender :
	public IThunderboltDescRender
{
public:
	virtual void Copy(IThunderboltDescRender &_in);

	virtual void CreateModel(LPCSTR m_name);
	virtual void DestroyModel();
	//private:
public:
	IRender_DetailModel*		l_model;
};

