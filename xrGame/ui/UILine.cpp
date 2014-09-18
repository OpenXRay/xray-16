// File:		UILine.cpp
// Description:	Single text line
// Created:		05.04.2005
// Author:		Serge Vynnycheko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World


#include "StdAfx.h"
#include "UILine.h"
#include "uilinestd.h"
#include "ui_base.h"

//#define LOG_ALL_LINES
#ifdef LOG_ALL_LINES
	int ListLinesCount = 0;
	struct DBGList{
		CUILine*		wnd;
		int				num;
	};
	xr_vector<DBGList>	dbg_list_lines;
	void dump_list_lines(){
		Msg("------Total  Lines %d",dbg_list_lines.size());
		xr_vector<DBGList>::iterator _it = dbg_list_lines.begin();
		for(;_it!=dbg_list_lines.end();++_it)
			Msg("--leak detected ---- Line = %d",(*_it).num);
	}
#else
	void dump_list_lines(){}
#endif

CUILine::CUILine(){
	m_tmpLine = NULL;
#ifdef LOG_ALL_LINES
	ListLinesCount++;
	dbg_list_lines.push_back(DBGList());
	dbg_list_lines.back().wnd = this;
	dbg_list_lines.back().num = ListLinesCount;
#endif
}

CUILine::~CUILine(){
	xr_delete(m_tmpLine);

#ifdef LOG_ALL_LINES
	xr_vector<DBGList>::iterator _it = dbg_list_lines.begin();
	bool bOK = false;
	for(;_it!=dbg_list_lines.end();++_it){
		if((*_it).wnd == this){
			bOK = true;
			dbg_list_lines.erase(_it);
			break;
		}
	}
	if(!bOK)
		Msg("CUILine::~CUILine()!!!!!!!!!!!!!!!!!!!!!!! cannot find window in list");
#endif
}

CUILine::CUILine(const CUILine& other){
	m_subLines = other.m_subLines;
	m_tmpLine = NULL;
#ifdef LOG_ALL_LINES
	ListLinesCount++;
	dbg_list_lines.push_back(DBGList());
	dbg_list_lines.back().wnd = this;
	dbg_list_lines.back().num = ListLinesCount;
#endif
}

CUILine& CUILine::operator =(const CUILine& other){
	m_subLines = other.m_subLines;
	xr_delete(m_tmpLine);
	return (*this);
}

void CUILine::AddSubLine(const xr_string& str, u32 color){
	CUISubLine sline;
	sline.m_color = color;
	sline.m_text = str;
	m_subLines.push_back(sline);
}

void CUILine::AddSubLine(const char* str, u32 color){
	CUISubLine sline;
	sline.m_color = color;
	sline.m_text = str;
	m_subLines.push_back(sline);
}

void CUILine::AddSubLine(const CUISubLine* subLine){
	m_subLines.push_back(*subLine);
}

void CUILine::Clear(){
	m_subLines.clear();
}

void CUILine::ProcessNewLines()
{
	for (u32 i=0; i < m_subLines.size(); i++){
		StrSize pos = m_subLines[i].m_text.find("\\n");
//		if (pos != npos)
//			pos = m_subLines[i].m_text.find('\r');

		if (pos != npos)
		{
			CUISubLine sbLine;
			if (pos)
                sbLine = *m_subLines[i].Cut2Pos((int)pos-1);
			sbLine.m_last_in_line = true;
			m_subLines.insert(m_subLines.begin()+i, sbLine);
			m_subLines[i+1].m_text.erase(0,2);
			if (m_subLines[i+1].m_text.empty()){
				m_subLines.erase(m_subLines.begin()+i+1);
			}
		}
	}
}

void CUILine::Draw(CGameFont* pFont, float x, float y) const{
	float length = 0;
	int size = m_subLines.size();

	for (int i=0; i<size; i++)
	{
		m_subLines[i].Draw(pFont, x+length, y);
		float ll = pFont->SizeOf_(m_subLines[i].m_text.c_str()); //. all ok
		UI().ClientToScreenScaledWidth(ll);
		length	+= ll;
	}
}

int CUILine::GetSize(){
	int sz = 0;
	int size = m_subLines.size();
	for (int i=0; i<size; i++)
		sz += (int)m_subLines[i].m_text.size();

	return sz;
}

const CUILine* CUILine::GetEmptyLine(){
	xr_delete(m_tmpLine);
	m_tmpLine = xr_new<CUILine>();

    return m_tmpLine;
}

const CUILine* CUILine::Cut2Pos(Position& pos, bool to_first){
	xr_delete(m_tmpLine);
	m_tmpLine = xr_new<CUILine>();

	int last;

	if (to_first || !pos.is_separated())
		last = pos.curr_subline - 1;
	else
		last = pos.curr_subline;

	for (int i = 0; i<= last; i++)
	{
		m_tmpLine->AddSubLine(&m_subLines[i]);

		if (m_subLines[i].m_last_in_line) // check if this subline must be last in line
		{
			for (int j = 0; j<= i; j++)
				m_subLines.erase(m_subLines.begin());
			return m_tmpLine;
		}
	}

	if (to_first)
		m_tmpLine->AddSubLine(m_subLines[last + 1].Cut2Pos(pos.word_1.last_space()));
	else
		m_tmpLine->AddSubLine(m_subLines[last + 1].Cut2Pos(pos.word_2.last_space()));

	for (int i = 0; i<= last; i++)
        m_subLines.erase(m_subLines.begin());

    return m_tmpLine;
}

