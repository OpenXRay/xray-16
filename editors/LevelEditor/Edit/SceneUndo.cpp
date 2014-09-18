//----------------------------------------------------
// file: SceneUndo.cpp
//
// Tanya M. : 265-74-96
//            265-86-00
//            251-46-69
//            251-46-70
//            251-46-76
//            251-46-77
//            251-46-78
//            264-89-00
//
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "Scene.h"
#include "../ECore/Editor/EditorPreferences.h"
#include "../ECore/Editor/ui_main.h"

//----------------------------------------------------

void EScene::UndoClear()
{
	while( !m_RedoStack.empty() ){
		unlink( m_RedoStack.back().m_FileName );
		m_RedoStack.pop_back(); }
	while( !m_UndoStack.empty() ){
		unlink( m_UndoStack.back().m_FileName );
		m_UndoStack.pop_back(); }
}

void EScene::UndoSave()
{
    if (UI->GetEState()!=esEditScene) return;
    Modified();
    UI->RedrawScene();

    if (0==EPrefs->scene_undo_level) return;

	UndoItem item;
	GetTempFileName( FS.get_path(_temp_)->m_Path, "undo", 0, item.m_FileName );

	Save( item.m_FileName, true, true );
	m_UndoStack.push_back( item );

	while( !m_RedoStack.empty() ){
		unlink( m_RedoStack.back().m_FileName );
		m_RedoStack.pop_back(); }

	if( m_UndoStack.size() > EPrefs->scene_undo_level ){
		unlink( m_UndoStack.front().m_FileName );
		m_UndoStack.pop_front(); }
}

bool EScene::Undo()
{
//	if( !m_UndoStack.empty() ){
	if( m_UndoStack.size()>1 ){
		m_RedoStack.push_back( m_UndoStack.back() );
		m_UndoStack.pop_back();

		if( m_RedoStack.size() > EPrefs->scene_undo_level ){
			unlink( m_RedoStack.front().m_FileName );
			m_RedoStack.pop_front();
        }

		if( !m_UndoStack.empty() ){
			Unload(TRUE);
         	Load( m_UndoStack.back().m_FileName, true );
        }

        UI->UpdateScene();
        Modified();

		return true;
	}
	return false;
}

bool EScene::Redo()
{
	if( !m_RedoStack.empty() ){
        Unload(TRUE);
		Load( m_RedoStack.back().m_FileName, true );

		m_UndoStack.push_back( m_RedoStack.back() );
		m_RedoStack.pop_back();

		if( m_UndoStack.size() > EPrefs->scene_undo_level ){
			unlink( m_UndoStack.front().m_FileName );
			m_UndoStack.pop_front(); }

        UI->UpdateScene();
        Modified();

		return true;
	}
	return false;
}

//----------------------------------------------------

