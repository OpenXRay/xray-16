#include "stdafx.h"
#pragma hdrstop

#include "Scene.h"
#include "ui_levelmain.h"
//------------------------------------------------------------------------------

void EScene::SelectObjects( bool flag, ObjClassID classfilter )
{
    if (classfilter==OBJCLASS_DUMMY){
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; _I++)
            if (_I->second)	_I->second->SelectObjects(flag);
    }else{
        ESceneToolBase* mt = GetTool(classfilter);
        if (mt)
        	mt->SelectObjects(flag);
    }

    UI->RedrawScene();
}
//------------------------------------------------------------------------------

int EScene::FrustumSelect( int flag, ObjClassID classfilter )
{
	CFrustum frustum;
	int count = 0;
    if (!LUI->SelectionFrustum(frustum)) return 0;

    if (classfilter==OBJCLASS_DUMMY)
    {
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; ++_I)
            if (_I->second)	count+=_I->second->FrustumSelect(flag,frustum);
    }else{
        ESceneToolBase* mt = GetTool(classfilter);
        if (mt)
        	count+=mt->FrustumSelect(flag,frustum);
    }
    
    UI->RedrawScene();
	return count;
}
//------------------------------------------------------------------------------

void EScene::InvertSelection( ObjClassID classfilter )
{
    if (classfilter==OBJCLASS_DUMMY)
    {
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; ++_I)
            if (_I->second)	_I->second->InvertSelection();
    }else
    {
        ESceneToolBase* mt 	= GetTool(classfilter);
        if (mt)
        	mt->InvertSelection();
    }
    UI->RedrawScene();
}
//------------------------------------------------------------------------------

void EScene::RemoveSelection( ObjClassID classfilter )
{
    if (classfilter==OBJCLASS_DUMMY){
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; _I++)
            if (_I->second&&_I->second->IsEditable())	_I->second->RemoveSelection();
    }else{
        ESceneToolBase* 		mt = GetTool(classfilter);
        if (mt&&mt->IsEditable()) 	mt->RemoveSelection();
    }
    UI->UpdateScene	(true);
}
//------------------------------------------------------------------------------

int EScene::SelectionCount(bool testflag, ObjClassID classfilter)
{
	int count = 0;

    if (classfilter==OBJCLASS_DUMMY){
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; _I++)
            if (_I->second)	count+=_I->second->SelectionCount(testflag);
    }else{
        ESceneToolBase* mt = GetTool(classfilter);
        if (mt) 			count+=mt->SelectionCount(testflag);
    }

    return count;
}
//------------------------------------------------------------------------------

bool EScene::ContainsObject( CCustomObject* object, ObjClassID classfilter ) 
{
	VERIFY( object );
	VERIFY( m_Valid );
    ObjectList& lst 	= ListObj(classfilter);
    ObjectIt it 		= std::find(lst.begin(), lst.end(), object);
    if (it!=lst.end())	return true;
    return false;
}
//------------------------------------------------------------------------------

int EScene::ObjCount()
{
	int cnt = 0;
    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();
    for (; _I!=_E; _I++){
    	ESceneCustomOTool* mt = dynamic_cast<ESceneCustomOTool*>(_I->second);
		if (mt)
        	cnt+=mt->ObjCount();
    }
	return cnt;
}
//------------------------------------------------------------------------------

void EScene::ShowObjects( bool flag, ObjClassID classfilter, bool bAllowSelectionFlag, bool bSelFlag )
{
    if (classfilter==OBJCLASS_DUMMY){
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; _I++)
            if (_I->second)	_I->second->ShowObjects(flag, bAllowSelectionFlag, bSelFlag);
    }else{
        ESceneToolBase* mt = GetTool(classfilter);
        if (mt)
        	mt->ShowObjects(flag, bAllowSelectionFlag, bSelFlag);
    }
    UI->RedrawScene();
}
//------------------------------------------------------------------------------

void EScene::SynchronizeObjects()
{
    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();
    for (; _I!=_E; _I++)
        if (_I->second)	_I->second->OnSynchronize();
}
//------------------------------------------------------------------------------

void EScene::ZoomExtents( ObjClassID cls, BOOL bSel )
{
	Fbox BB;	BB.invalidate();
    if (cls==OBJCLASS_DUMMY){
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; _I++)
            if (_I->second){
            	Fbox bb; 			bb.invalidate();
            	_I->second->GetBBox	(bb,bSel);
                if (bb.is_valid()) 	BB.merge(bb);
            }
    }else{
        ESceneToolBase* mt = GetTool(cls);
        if (mt) 			mt->GetBBox(BB,bSel);
    }
    if (BB.is_valid()) EDevice.m_Camera.ZoomExtents(BB);
    else ELog.Msg(mtError,"Can't calculate bounding box. Nothing selected or some object unsupported this function.");
}
//------------------------------------------------------------------------------

