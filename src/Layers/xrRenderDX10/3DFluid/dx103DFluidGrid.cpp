#include "stdafx.h"
#include "dx103DFluidGrid.h"
#include "Layers/xrRender/BufferUtils.h"

struct VS_INPUT_FLUIDSIM_STRUCT
{
    Fvector Pos; // Clip space position for slice vertices
    Fvector Tex; // Cell coordinates in 0-"texture dimension" range
};

namespace
{ //	namespace start

inline void ComputeRowColsForFlat3DTexture(int depth, int* outCols, int* outRows)
{
    // Compute # of m_iRows and m_iCols for a "flat 3D-texture" configuration
    // (in this configuration all the slices in the volume are spread in a single 2D texture)
    int m_iRows = (int)floorf(_sqrt((float)depth));
    int m_iCols = m_iRows;
    while (m_iRows * m_iCols < depth)
    {
        m_iCols++;
    }
    VERIFY(m_iRows * m_iCols >= depth);

    *outCols = m_iCols;
    *outRows = m_iRows;
}
} //	namespace end

#define VERTICES_PER_SLICE 6
#define VERTICES_PER_LINE 2
#define LINES_PER_SLICE 4

dx103DFluidGrid::dx103DFluidGrid() {}
dx103DFluidGrid::~dx103DFluidGrid()
{
    DestroyVertexBuffers();
}

void dx103DFluidGrid::Initialize(int gridWidth, int gridHeight, int gridDepth)
{
    m_vDim[0] = gridWidth;
    m_vDim[1] = gridHeight;
    m_vDim[2] = gridDepth;

    m_iMaxDim = _max(_max(m_vDim[0], m_vDim[1]), m_vDim[2]);

    ComputeRowColsForFlat3DTexture(m_vDim[2], &m_iCols, &m_iRows);

    CreateVertexBuffers();
}

void dx103DFluidGrid::CreateVertexBuffers()
{
    // Create layout
    // D3Dxx_INPUT_ELEMENT_DESC layoutDesc[] =
    //{
    //	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0, 0, D3Dxx_INPUT_PER_VERTEX_DATA, 0 },
    //	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0,12, D3Dxx_INPUT_PER_VERTEX_DATA, 0 },
    //};

    static D3DVERTEXELEMENT9 layoutDesc[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, D3DDECL_END()};

    u32 vSize = GetDeclVertexSize(layoutDesc, 0);

    // UINT numElements = sizeof(layoutDesc)/sizeof(layoutDesc[0]);
    // CreateLayout( layoutDesc, numElements, technique, &layout);

    int index = 0;
    VS_INPUT_FLUIDSIM_STRUCT* renderQuad = NULL;
    VS_INPUT_FLUIDSIM_STRUCT* slices = NULL;
    VS_INPUT_FLUIDSIM_STRUCT* boundarySlices = NULL;
    VS_INPUT_FLUIDSIM_STRUCT* boundaryLines = NULL;

    m_iNumVerticesRenderQuad = VERTICES_PER_SLICE * m_vDim[2];
    m_pRenderQuadBuffer.Create(vSize * m_iNumVerticesRenderQuad);
    renderQuad = static_cast<VS_INPUT_FLUIDSIM_STRUCT*>(m_pRenderQuadBuffer.Map());

    m_iNumVerticesSlices = VERTICES_PER_SLICE * (m_vDim[2] - 2);
    m_pSlicesBuffer.Create(vSize * m_iNumVerticesSlices);
    slices = static_cast<VS_INPUT_FLUIDSIM_STRUCT*>(m_pSlicesBuffer.Map());

    m_iNumVerticesBoundarySlices = VERTICES_PER_SLICE * 2;
    m_pBoundarySlicesBuffer.Create(vSize * m_iNumVerticesBoundarySlices);
    boundarySlices = static_cast<VS_INPUT_FLUIDSIM_STRUCT*>(m_pBoundarySlicesBuffer.Map());

    m_iNumVerticesBoundaryLines = VERTICES_PER_LINE * LINES_PER_SLICE * (m_vDim[2]);
    m_pBoundaryLinesBuffer.Create(vSize * m_iNumVerticesBoundaryLines);
    boundaryLines = static_cast<VS_INPUT_FLUIDSIM_STRUCT*>(m_pBoundaryLinesBuffer.Map());

    VERIFY(renderQuad && slices && boundarySlices && boundaryLines);

    // Vertex buffer for "m_vDim[2]" quads to draw all the slices of the 3D-texture as a flat 3D-texture
    // (used to draw all the individual slices at once to the screen buffer)
    index = 0;
    for (int z = 0; z < m_vDim[2]; z++)
        InitScreenSlice(&renderQuad, z, index);

    // CreateVertexBuffer(sizeof(VS_INPUT_FLUIDSIM_STRUCT)*numVerticesRenderQuad,
    //	D3Dxx_BIND_VERTEX_BUFFER, &renderQuadBuffer, renderQuad, numVerticesRenderQuad));
    m_pRenderQuadBuffer.Unmap(true);
    m_GeomRenderQuad.create(layoutDesc, m_pRenderQuadBuffer, 0);

    // Vertex buffer for "m_vDim[2]" quads to draw all the slices to a 3D texture
    // (a Geometry Shader is used to send each quad to the appropriate slice)
    index = 0;
    for (int z = 1; z < m_vDim[2] - 1; z++)
        InitSlice(z, &slices, index);
    VERIFY(index == m_iNumVerticesSlices);
    // V_RETURN(CreateVertexBuffer(sizeof(VS_INPUT_FLUIDSIM_STRUCT)*numVerticesSlices,
    //	D3Dxx_BIND_VERTEX_BUFFER, &slicesBuffer, slices , numVerticesSlices));
    m_pSlicesBuffer.Unmap(true);
    m_GeomSlices.create(layoutDesc, m_pSlicesBuffer, 0);

    // Vertex buffers for boundary geometry
    //   2 boundary slices
    index = 0;
    InitBoundaryQuads(&boundarySlices, index);
    VERIFY(index == m_iNumVerticesBoundarySlices);
    // V_RETURN(CreateVertexBuffer(sizeof(VS_INPUT_FLUIDSIM_STRUCT)*numVerticesBoundarySlices,
    //	D3Dxx_BIND_VERTEX_BUFFER, &boundarySlicesBuffer, boundarySlices, numVerticesBoundarySlices));
    m_pBoundarySlicesBuffer.Unmap(true);
    m_GeomBoundarySlices.create(layoutDesc, m_pBoundarySlicesBuffer, 0);

    //   ( 4 * "m_vDim[2]" ) boundary lines
    index = 0;
    InitBoundaryLines(&boundaryLines, index);
    VERIFY(index == m_iNumVerticesBoundaryLines);
    // V_RETURN(CreateVertexBuffer(sizeof(VS_INPUT_FLUIDSIM_STRUCT)*numVerticesBoundaryLines,
    //	D3Dxx_BIND_VERTEX_BUFFER, &boundaryLinesBuffer, boundaryLines, numVerticesBoundaryLines));
    m_pBoundaryLinesBuffer.Unmap(true);
    m_GeomBoundaryLines.create(layoutDesc, m_pBoundaryLinesBuffer, 0);
}

void dx103DFluidGrid::DestroyVertexBuffers()
{
    m_pRenderQuadBuffer.Release();
    m_pSlicesBuffer.Release();
    m_pBoundarySlicesBuffer.Release();
    m_pBoundaryLinesBuffer.Release();
}

void dx103DFluidGrid::InitScreenSlice(VS_INPUT_FLUIDSIM_STRUCT** vertices, int z, int& index)
{

    // compute the offset (px, py) in the "flat 3D-texture" space for the slice with given 'z' coordinate
    const int column = z % m_iCols;
    const int row = (int)floorf((float)(z / m_iCols));
    const int px = column * m_vDim[0];
    const int py = row * m_vDim[1];

    const float w = float(m_vDim[0]);
    const float h = float(m_vDim[1]);

    const float Width = float(m_iCols * m_vDim[0]);
    const float Height = float(m_iRows * m_vDim[1]);

    const VS_INPUT_FLUIDSIM_STRUCT tempVertex1 =
    {
        { px * 2.0f / Width - 1.0f, -(py * 2.0f / Height) + 1.0f, 0.0f },
        { 0, 0, float(z) }
    };
    const VS_INPUT_FLUIDSIM_STRUCT tempVertex2 =
    {
        { (px + w) * 2.0f / Width - 1.0f, -((py) * 2.0f / Height) + 1.0f, 0.0f },
        { w, 0, float(z) }
    };
    const VS_INPUT_FLUIDSIM_STRUCT tempVertex3 =
    {
        { (px + w) * 2.0f / Width - 1.0f, -((py + h) * 2.0f / Height) + 1.0f, 0.0f },
        { w, h, float(z) }
    };
    const VS_INPUT_FLUIDSIM_STRUCT tempVertex4 =
    {
        { (px) * 2.0f / Width - 1.0f, -((py + h) * 2.0f / Height) + 1.0f, 0.0f },
        { 0, h, float(z) }
    };

    (*vertices)[index++] = tempVertex1;
    (*vertices)[index++] = tempVertex2;
    (*vertices)[index++] = tempVertex3;
    (*vertices)[index++] = tempVertex1;
    (*vertices)[index++] = tempVertex3;
    (*vertices)[index++] = tempVertex4;
}

void dx103DFluidGrid::InitSlice(int z, VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index)
{
    const int w = m_vDim[0];
    const int h = m_vDim[1];

    const VS_INPUT_FLUIDSIM_STRUCT tempVertex1 =
    {
        { 1 * 2.0f / w - 1.0f, -1 * 2.0f / h + 1.0f, 0.0f },
        { 1.0f, 1.0f, float(z) }
    };
    const VS_INPUT_FLUIDSIM_STRUCT tempVertex2 =
    {
        { (w - 1.0f) * 2.0f / w - 1.0f, -1 * 2.0f / h + 1.0f, 0.0f },
        { (w - 1.0f), 1.0f, float(z) }
    };
    const VS_INPUT_FLUIDSIM_STRUCT tempVertex3 =
    {
        { (w - 1.0f) * 2.0f / w - 1.0f, -(h - 1) * 2.0f / h + 1.0f, 0.0f },
        { (w - 1.0f), (h - 1.0f), float(z) }
    };
    const VS_INPUT_FLUIDSIM_STRUCT tempVertex4 =
    {
        { 1 * 2.0f / w - 1.0f, -(h - 1.0f) * 2.0f / h + 1.0f, 0.0f },
        { 1.0f, (h - 1.0f), float(z) }
    };

    (*vertices)[index++] = tempVertex1;
    (*vertices)[index++] = tempVertex2;
    (*vertices)[index++] = tempVertex3;
    (*vertices)[index++] = tempVertex1;
    (*vertices)[index++] = tempVertex3;
    (*vertices)[index++] = tempVertex4;
}

void dx103DFluidGrid::InitLine(
    float x1, float y1, float x2, float y2, int z, VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index)
{
    int w = m_vDim[0];
    int h = m_vDim[1];

    (*vertices)[index++] =
    {
        { x1 * 2.0f / w - 1.0f, -y1 * 2.0f / h + 1.0f, 0.5f },
        { 0.0f, 0.0f, float(z) }
    };

    (*vertices)[index++] =
    {
        { x2 * 2.0f / w - 1.0f, -y2 * 2.0f / h + 1.0f, 0.5f },
        { 0.0f, 0.0f, float(z) }
    };
}

void dx103DFluidGrid::InitBoundaryQuads(VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index)
{
    InitSlice(0, vertices, index);
    InitSlice(m_vDim[2] - 1, vertices, index);
}

void dx103DFluidGrid::InitBoundaryLines(VS_INPUT_FLUIDSIM_STRUCT** vertices, int& index)
{
    int w = m_vDim[0];
    int h = m_vDim[1];

    for (int z = 0; z < m_vDim[2]; z++)
    {
        // bottom
        InitLine(0.0f, 1.0f, float(w), 1.0f, z, vertices, index);
        // top
        InitLine(0.0f, float(h), float(w), float(h), z, vertices, index);
        // left
        InitLine(1.0f, 0.0f, 1.0f, float(h), z, vertices, index);
        // right
        InitLine(float(w), 0.0f, float(w), float(h), z, vertices, index);
    }
}

void dx103DFluidGrid::DrawSlices(void)
{
    // UINT stride[1] = { sizeof(VS_INPUT_FLUIDSIM_STRUCT) };
    // UINT offset[1] = { 0 };
    // DrawPrimitive( D3Dxx_PRIMITIVE_TOPOLOGY_TRIANGLELIST, layout, &slicesBuffer,
    //	stride, offset, 0, numVerticesSlices );

    RCache.set_Geometry(m_GeomSlices);
    RCache.Render(D3DPT_TRIANGLELIST, 0, m_iNumVerticesSlices / 3);
}

void dx103DFluidGrid::DrawSlicesToScreen(void)
{
    // UINT stride[1] = { sizeof(VS_INPUT_FLUIDSIM_STRUCT) };
    // UINT offset[1] = { 0 };
    // DrawPrimitive( D3Dxx_PRIMITIVE_TOPOLOGY_TRIANGLELIST, layout, &renderQuadBuffer,
    //	stride, offset, 0, numVerticesRenderQuad );

    RCache.set_Geometry(m_GeomRenderQuad);
    RCache.Render(D3DPT_TRIANGLELIST, 0, m_iNumVerticesRenderQuad / 3);
}

void dx103DFluidGrid::DrawBoundaryQuads(void)
{
    // UINT stride[1] = { sizeof(VS_INPUT_FLUIDSIM_STRUCT) };
    // UINT offset[1] = { 0 };
    // DrawPrimitive( D3Dxx_PRIMITIVE_TOPOLOGY_TRIANGLELIST, layout, &boundarySlicesBuffer,
    //	stride, offset, 0, numVerticesBoundarySlices );

    RCache.set_Geometry(m_GeomBoundarySlices);
    RCache.Render(D3DPT_TRIANGLELIST, 0, m_iNumVerticesBoundarySlices / 3);
}

void dx103DFluidGrid::DrawBoundaryLines(void)
{
    //	UINT stride[1] = { sizeof(VS_INPUT_FLUIDSIM_STRUCT) };
    //	UINT offset[1] = { 0 };
    //	DrawPrimitive( D3Dxx_PRIMITIVE_TOPOLOGY_LINELIST, layout, &boundaryLinesBuffer,
    //		stride, offset, 0, numVerticesBoundaryLines  );

    RCache.set_Geometry(m_GeomBoundaryLines);
    RCache.Render(D3DPT_TRIANGLELIST, 0, m_iNumVerticesBoundaryLines / 3);
}
