#ifndef	dx103DFluidGrid_included
#define	dx103DFluidGrid_included
#pragma once

struct VS_INPUT_FLUIDSIM_STRUCT;

class dx103DFluidGrid
{
public:
	dx103DFluidGrid();
	~dx103DFluidGrid();

	void	Initialize( int gridWidth, int gridHeight, int gridDepth);

	void	DrawSlices();
	void	DrawSlicesToScreen();
	void	DrawBoundaryQuads();
	void	DrawBoundaryLines();
	
private:
	void	CreateVertexBuffers();

	void	InitScreenSlice( VS_INPUT_FLUIDSIM_STRUCT** vertices, int z, int& index );
	void	InitSlice( int z, VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index );
	void	InitLine(float x1, float y1, float x2, float y2, int z,
					VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index);
	void	InitBoundaryQuads( VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index );
	void	InitBoundaryLines( VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index );

private:

	Ivector3	m_vDim;
	int			m_iMaxDim;
	int			m_iCols;
	int			m_iRows;

	ref_geom		m_GeomRenderQuad;
	ref_geom		m_GeomSlices;
	ref_geom		m_GeomBoundarySlices;
	ref_geom		m_GeomBoundaryLines;

	ID3DBuffer*	m_pRenderQuadBuffer;
	ID3DBuffer*	m_pSlicesBuffer;
	ID3DBuffer*	m_pBoundarySlicesBuffer;
	ID3DBuffer*	m_pBoundaryLinesBuffer;

	int			m_iNumVerticesRenderQuad;
	int			m_iNumVerticesSlices;
	int			m_iNumVerticesBoundarySlices;
	int			m_iNumVerticesBoundaryLines;
};

#endif	//	dx103DFluidGrid_included