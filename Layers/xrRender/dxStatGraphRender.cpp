#include "stdafx.h"
#include "dxStatGraphRender.h"

void dxStatGraphRender::Copy(IStatGraphRender&_in)
{
	*this = *((dxStatGraphRender*)&_in);
}

void dxStatGraphRender::OnDeviceCreate()
{
	hGeomLine.create(FVF::F_TL0uv,RCache.Vertex.Buffer(),RCache.Index.Buffer());
	hGeomTri.create(FVF::F_TL0uv,RCache.Vertex.Buffer(),RCache.QuadIB);
}

void dxStatGraphRender::OnDeviceDestroy()
{
	hGeomLine.destroy();
	hGeomTri.destroy();
}

void dxStatGraphRender::OnRender(CStatGraph &owner)
{
	RCache.OnFrameEnd();

	RenderBack(owner);

	u32			TriElem = 0;
	u32			LineElem = 0;
	for (CStatGraph::SubGraphVecIt it=owner.subgraphs.begin(); it!=owner.subgraphs.end(); it++)
	{
		switch (it->style)
		{
		case CStatGraph::stBar: 
			{
				TriElem += it->elements.size()*4;
			}break;
		case CStatGraph::stCurve: 
			{
				LineElem += it->elements.size()*2;
			}break;
		case CStatGraph::stBarLine: 
			{
				LineElem += it->elements.size()*4;
			}break;
		case CStatGraph::stPoint: 
			{
				//				TriElem += it->elements.size()*4;
			}break;
		};
	};

	u32			dwOffsetTri = 0, dwOffsetLine = 0,dwCount;
	FVF::TL0uv* pv_Tri_start	= NULL;
	FVF::TL0uv* pv_Tri;
	FVF::TL0uv* pv_Line_start	= NULL;
	FVF::TL0uv* pv_Line;

	if (TriElem)
	{
		pv_Tri_start = (FVF::TL0uv*)RCache.Vertex.Lock(TriElem,hGeomTri->vb_stride,dwOffsetTri);
		pv_Tri = pv_Tri_start;

		pv_Tri = pv_Tri_start;
		for (CStatGraph::SubGraphVecIt it=owner.subgraphs.begin(); it!=owner.subgraphs.end(); it++)
		{
			switch(it->style)
			{
			case CStatGraph::stBar:		RenderBars		(owner, &pv_Tri, &(it->elements));		break;
			};
		};
		dwCount 				= u32(pv_Tri-pv_Tri_start);
		RCache.Vertex.Unlock	(dwCount,hGeomTri->vb_stride);
		RCache.set_Geometry		(hGeomTri);
		RCache.Render	   		(D3DPT_TRIANGLELIST,dwOffsetTri,0, dwCount, 0, dwCount/2);
	};

	if (LineElem)
	{
		pv_Line_start = (FVF::TL0uv*)RCache.Vertex.Lock(LineElem,hGeomLine->vb_stride,dwOffsetLine);
		pv_Line = pv_Line_start;

		for (CStatGraph::SubGraphVecIt it=owner.subgraphs.begin(); it!=owner.subgraphs.end(); it++)
		{
			switch(it->style)
			{
			case CStatGraph::stCurve:	RenderLines		(owner, &pv_Line, &(it->elements));	break;
			case CStatGraph::stBarLine:	RenderBarLines	(owner, &pv_Line, &(it->elements));	break;
			};
		};

		dwCount 				= u32(pv_Line-pv_Line_start);
		RCache.Vertex.Unlock	(dwCount,hGeomLine->vb_stride);
		RCache.set_Geometry		(hGeomLine);
		RCache.Render	   		(D3DPT_LINELIST,dwOffsetLine,dwCount/2);
	};

	if (!owner.m_Markers.empty())
	{
		dwOffsetLine = 0;
		LineElem = owner.m_Markers.size()*2;

		pv_Line_start = (FVF::TL0uv*)RCache.Vertex.Lock(LineElem,hGeomLine->vb_stride,dwOffsetLine);
		pv_Line = pv_Line_start;

		RenderMarkers		(owner, &pv_Line, &(owner.m_Markers));

		dwCount 				= u32(pv_Line-pv_Line_start);
		RCache.Vertex.Unlock	(dwCount,hGeomLine->vb_stride);
		RCache.set_Geometry		(hGeomLine);
		RCache.Render	   		(D3DPT_LINELIST,dwOffsetLine,dwCount/2);
	}
}

void dxStatGraphRender::RenderBack(CStatGraph &owner)
{
	// draw back
	u32			dwOffset,dwCount;
	FVF::TL0uv* pv_start				= (FVF::TL0uv*)RCache.Vertex.Lock(4,hGeomTri->vb_stride,dwOffset);
	FVF::TL0uv* pv						= pv_start;
	// base rect
	pv->set					(owner.lt.x,owner.rb.y,owner.back_color); pv++;	// 0
	pv->set					(owner.lt.x,owner.lt.y,owner.back_color); pv++; 	// 1
	pv->set					(owner.rb.x,owner.rb.y,owner.back_color); pv++;	// 2
	pv->set					(owner.rb.x,owner.lt.y,owner.back_color); pv++;	// 3
	// render	
	dwCount 				= u32(pv-pv_start);
	RCache.Vertex.Unlock	(dwCount,hGeomTri->vb_stride);
	RCache.set_Geometry		(hGeomTri);
	RCache.Render	   		(D3DPT_TRIANGLELIST,dwOffset,0, dwCount, 0, dwCount/2);	

	//draw rect
	pv_start				= (FVF::TL0uv*)RCache.Vertex.Lock(5,hGeomLine->vb_stride,dwOffset);
	pv						= pv_start;
	// base rect
	pv->set					(owner.lt.x,owner.lt.y,owner.rect_color); pv++;	// 0
	pv->set					(owner.rb.x-1,owner.lt.y,owner.rect_color); pv++; 	// 1
	pv->set					(owner.rb.x-1,owner.rb.y,owner.rect_color); pv++;	// 2
	pv->set					(owner.lt.x,owner.rb.y,owner.rect_color); pv++;	// 3
	pv->set					(owner.lt.x,owner.lt.y,owner.rect_color); pv++;	// 0
	// render	
	dwCount 				= u32(pv-pv_start);
	RCache.Vertex.Unlock	(dwCount,hGeomLine->vb_stride);
	RCache.set_Geometry		(hGeomLine);
	RCache.Render	   		(D3DPT_LINESTRIP,dwOffset,4);

	// draw owner.grid
	float elem_factor	= float(owner.rb.y-owner.lt.y)/float(owner.mx-owner.mn);
	float base_y		= float(owner.rb.y)+(owner.mn*elem_factor);

	int PNum_H_LinesUp	= int((base_y - float(owner.lt.y)) / (owner.grid_step.y*elem_factor));
	int PNum_H_LinesDwn = u32((float(owner.rb.y) - base_y) / (owner.grid_step.y*elem_factor));
	int Num_H_LinesUp = (owner.grid.y < PNum_H_LinesUp) ? owner.grid.y : PNum_H_LinesUp;
	int Num_H_LinesDwn = (owner.grid.y < PNum_H_LinesUp) ? owner.grid.y : PNum_H_LinesDwn;

	pv_start	= (FVF::TL0uv*)RCache.Vertex.Lock(	2 + 2*owner.grid.x + Num_H_LinesUp*2 + Num_H_LinesDwn*2,
		hGeomLine->vb_stride,dwOffset);
	pv			= pv_start;
	// base Coordinate Line
	pv->set					(owner.lt.x, int(base_y), owner.base_color); pv++; // 0
	pv->set					(owner.rb.x, int(base_y), owner.base_color); pv++;	// 0    
	// owner.grid
	//    float grid_offs_x		= float(owner.rb.x-owner.lt.x)/float(owner.grid.x+1);
	//    float grid_offs_y		= float(owner.rb.y-owner.lt.y)/float(owner.grid.y+1);
	for (int g_x=1; g_x<=owner.grid.x; g_x++)
	{
		pv->set				(int(owner.lt.x + g_x*owner.grid_step.x*elem_factor),owner.lt.y,owner.grid_color); pv++; 	
		pv->set				(int(owner.lt.x + g_x*owner.grid_step.x*elem_factor),owner.rb.y,owner.grid_color); pv++; 	
	}
	for (int g_y=1; g_y<=Num_H_LinesDwn; g_y++)
	{
		pv->set				(owner.lt.x,int(base_y+g_y*owner.grid_step.y*elem_factor),owner.grid_color); pv++;
		pv->set				(owner.rb.x,int(base_y+g_y*owner.grid_step.y*elem_factor),owner.grid_color); pv++;
	};

	for (g_y=1; g_y<=Num_H_LinesUp; g_y++)
	{									
		pv->set				(owner.lt.x,int(base_y-g_y*owner.grid_step.y*elem_factor),owner.grid_color); pv++; 	
		pv->set				(owner.rb.x,int(base_y-g_y*owner.grid_step.y*elem_factor),owner.grid_color); pv++; 	
	}    	


	//    for (int g_y=1; g_y<=owner.grid.y; g_y++){
	//	    pv->set				(owner.lt.x,iFloor(g_y*grid_offs_y+owner.lt.y),owner.grid_color); pv++; 	
	//	    pv->set				(owner.rb.x,iFloor(g_y*grid_offs_y+owner.lt.y),owner.grid_color); pv++; 	
	//	}

	dwCount 				= u32(pv-pv_start);
	RCache.Vertex.Unlock	(dwCount,hGeomLine->vb_stride);
	RCache.set_Geometry		(hGeomLine);
	RCache.Render	   		(D3DPT_LINELIST,dwOffset,dwCount/2);
}

void dxStatGraphRender::RenderBars(CStatGraph &owner, FVF::TL0uv** ppv, CStatGraph::ElementsDeq* pelements)
{
	float elem_offs		= float(owner.rb.x-owner.lt.x)/owner.max_item_count;
	float elem_factor	= float(owner.rb.y-owner.lt.y)/float(owner.mx-owner.mn);
	float base_y		= float(owner.rb.y)+(owner.mn*elem_factor);

	float column_width = elem_offs;
	if (column_width > 1) column_width--;
	for (CStatGraph::ElementsDeqIt it=pelements->begin(); it!=pelements->end(); it++)
	{
		float X		= float(it-pelements->begin())*elem_offs+owner.lt.x;
		float Y0	= base_y;
		float Y1	= base_y - it->data*elem_factor;

		if (Y1 > Y0)
		{
			(*ppv)->set		(X,Y1,it->color); (*ppv)++;
			(*ppv)->set		(X,Y0,it->color); (*ppv)++;
			(*ppv)->set		(X+column_width,Y1,it->color); (*ppv)++;
			(*ppv)->set		(X+column_width,Y0,it->color); (*ppv)++;
		}
		else
		{
			(*ppv)->set		(X,Y0,it->color); (*ppv)++;
			(*ppv)->set		(X,Y1,it->color); (*ppv)++;
			(*ppv)->set		(X+column_width,Y0,it->color); (*ppv)++;
			(*ppv)->set		(X+column_width,Y1,it->color); (*ppv)++;
		};
	};
}

void dxStatGraphRender::RenderLines(CStatGraph &owner, FVF::TL0uv** ppv, CStatGraph::ElementsDeq* pelements )
{
	float elem_offs		= float(owner.rb.x-owner.lt.x)/owner.max_item_count;
	float elem_factor	= float(owner.rb.y-owner.lt.y)/float(owner.mx-owner.mn);
	float base_y		= float(owner.rb.y)+(owner.mn*elem_factor);

	for (CStatGraph::ElementsDeqIt it=pelements->begin()+1;  it!=pelements->end() && it!=pelements->end()+1; it++)
	{
		CStatGraph::ElementsDeqIt it_prev = it-1;
		float X0	= float(it_prev-pelements->begin())*elem_offs+owner.lt.x;
		float Y0	= base_y-it_prev->data*elem_factor;
		(*ppv)->set		(X0,Y0,it->color); (*ppv)++;
		float X1	= float(it-pelements->begin())*elem_offs+owner.lt.x;
		float Y1	= base_y-it->data*elem_factor;
		(*ppv)->set		(X1,Y1,it->color); (*ppv)++;
	}
};

void dxStatGraphRender::RenderBarLines(CStatGraph &owner, FVF::TL0uv** ppv, CStatGraph::ElementsDeq* pelements )
{
	float elem_offs		= float(owner.rb.x-owner.lt.x)/owner.max_item_count;
	float elem_factor	= float(owner.rb.y-owner.lt.y)/float(owner.mx-owner.mn);
	float base_y		= float(owner.rb.y)+(owner.mn*elem_factor);

	for (CStatGraph::ElementsDeqIt it=pelements->begin()+1; it!=pelements->end() && it!=pelements->end()+1; it++)
	{
		CStatGraph::ElementsDeqIt it_prev = it-1;
		float X0	= float(it_prev-pelements->begin())*elem_offs+owner.lt.x+elem_offs;
		float Y0	= base_y-it_prev->data*elem_factor;
		(*ppv)->set		(X0,Y0,it->color); (*ppv)++;
		float X1	= float(it-pelements->begin())*elem_offs+owner.lt.x;
		float Y1	= base_y-it->data*elem_factor;
		(*ppv)->set		(X1,Y1,it->color); (*ppv)++;
		(*ppv)->set		(X1,Y1,it->color); (*ppv)++;
		X1 += elem_offs;
		(*ppv)->set		(X1,Y1,it->color); (*ppv)++;
	}
};

void dxStatGraphRender::RenderMarkers(CStatGraph &owner, FVF::TL0uv** ppv, CStatGraph::MarkersDeq* pmarkers )
{
	float elem_offs		= float(owner.rb.x-owner.lt.x)/owner.max_item_count;
	float elem_factor	= float(owner.rb.y-owner.lt.y)/float(owner.mx-owner.mn);
	float base_y		= float(owner.rb.y)+(owner.mn*elem_factor);

	for (CStatGraph::MarkersDeqIt it=pmarkers->begin();  it!=pmarkers->end() && it!=pmarkers->end()+1; it++)
	{
		CStatGraph::SMarker &CurMarker = *it;
		float X0 = 0, Y0 = 0, X1 = 0, Y1 = 0;
		switch (CurMarker.m_eStyle)
		{
		case CStatGraph::stVert:
			{
				X0 = CurMarker.m_fPos*elem_offs+owner.lt.x;
				clamp(X0, float(owner.lt.x), float(owner.rb.x));
				X1 = X0;
				Y0 = float(owner.lt.y);
				Y1 = float(owner.rb.y);
			}break;
		case CStatGraph::stHor:
			{
				X0 = float(owner.lt.x);
				X1 = float(owner.rb.x);
				Y0 = base_y - CurMarker.m_fPos*elem_factor;
				clamp(Y0, float(owner.lt.y), float(owner.rb.y));
				Y1 = Y0;
			}break;
		}
		(*ppv)->set		(X0,Y0,CurMarker.m_dwColor); (*ppv)++;
		(*ppv)->set		(X1,Y1,CurMarker.m_dwColor); (*ppv)++;
	}
}