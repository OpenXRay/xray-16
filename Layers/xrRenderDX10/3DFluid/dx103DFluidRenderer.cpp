#include "stdafx.h"
#include "dx103DFluidRenderer.h"

#include "dx103DFluidBlenders.h"

#include "../dx10BufferUtils.h"
#include "../../xrRender/dxRenderDeviceRender.h"

#include "dx103DFluidData.h"

struct VsInput
{
	D3DXVECTOR3 pos;  
};

namespace
{
	// For render call
	//pZNearVar = pEffect->GetVariableByName("ZNear")->AsScalar();
	shared_str	strZNear("ZNear");
	//pZFarVar = pEffect->GetVariableByName("ZFar")->AsScalar();
	shared_str	strZFar("ZFar");
	//pGridScaleFactorVar = pEffect->GetVariableByName( "gridScaleFactor")->AsScalar();
	shared_str	strGridScaleFactor("gridScaleFactor");
	//pEyeOnGridVar = pEffect->GetVariableByName("eyeOnGrid")->AsVector();
	shared_str	strEyeOnGrid("eyeOnGrid");
	//pWorldViewProjectionVar = pEffect->GetVariableByName("WorldViewProjection")->AsMatrix();
	shared_str	strWorldViewProjection("WorldViewProjection");
	//pInvWorldViewProjectionVar = pEffect->GetVariableByName("InvWorldViewProjection")->AsMatrix();
	shared_str	strInvWorldViewProjection("InvWorldViewProjection");
	//pRTWidthVar = pEffect->GetVariableByName("RTWidth")->AsScalar();
	shared_str	strRTWidth("RTWidth");
	//pRTHeightVar = pEffect->GetVariableByName("RTHeight")->AsScalar();
	shared_str	strRTHeight("RTHeight");

	shared_str	strDiffuseLight("DiffuseLight");
}

LPCSTR			dx103DFluidRenderer::m_pRTNames[ RRT_NumRT ] = 
{
	"$user$rayDataTex",
	"$user$rayDataTexSmall",
	"$user$rayCastTex",
	"$user$edgeTex"
};

LPCSTR			dx103DFluidRenderer::m_pResourceRTNames[ RRT_NumRT ] = 
{
	"rayDataTex",
	"rayDataTexSmall",
	"rayCastTex",
	"edgeTex"
};

dx103DFluidRenderer::dx103DFluidRenderer():
	m_bInited(false)
{
	RTFormats[RRT_RayDataTex] = D3DFMT_A32B32G32R32F;
	RTFormats[RRT_RayDataTexSmall] = D3DFMT_A32B32G32R32F;
	RTFormats[RRT_RayCastTex] = D3DFMT_A32B32G32R32F;
	RTFormats[RRT_EdgeTex] = D3DFMT_R32F;
}

dx103DFluidRenderer::~dx103DFluidRenderer()
{
	Destroy();
}

void dx103DFluidRenderer::Initialize(int gridWidth, int gridHeight, int gridDepth)
{
	Destroy();

	m_vGridDim[0] = float(gridWidth);
	m_vGridDim[1] = float(gridHeight);
	m_vGridDim[2] = float(gridDepth);

	m_fMaxDim = _max( _max( m_vGridDim[0], m_vGridDim[1] ), m_vGridDim[2] );

	// Initialize the grid offset matrix
	{
		// Make a scale matrix to scale the unit-sided box to be unit-length on the 
		//  side/s with maximum dimension 
		D3DXMATRIX scaleM;
		D3DXMatrixIdentity(&scaleM);
		D3DXMatrixScaling(&scaleM, m_vGridDim[0] / m_fMaxDim, m_vGridDim[1] / m_fMaxDim, m_vGridDim[2] / m_fMaxDim);
		// offset grid to be centered at origin
		D3DXMATRIX translationM;
		D3DXMatrixTranslation(&translationM, -0.5, -0.5, -0.5);

		m_gridMatrix = translationM * scaleM;
		//m_gridMatrix.scale(m_vGridDim[0] / m_fMaxDim, m_vGridDim[1] / m_fMaxDim, m_vGridDim[2] / m_fMaxDim);
		//m_gridMatrix.translate_over(-0.5, -0.5, -0.5);
	}

	InitShaders();
	CreateGridBox();
	CreateScreenQuad();
	CreateJitterTexture();
	CreateHHGGTexture();

	m_bInited = true;
}

void dx103DFluidRenderer::Destroy()
{
	if (!m_bInited) return;

	//createJitterTexture();
	m_JitterTexture = 0;
	m_HHGGTexture = 0;

	//createScreenQuad();
	m_GeomQuadVertex = 0;
	_RELEASE(m_pQuadVertexBuffer);

	//createGridBox();
	m_GeomGridBox = 0;
	_RELEASE(m_pGridBoxVertexBuffer);
	_RELEASE(m_pGridBoxIndexBuffer);

	DestroyShaders();
}

void dx103DFluidRenderer::InitShaders()
{

	{
		CBlender_fluid_raydata	Blender;
		ref_shader				shader;
		shader.create(&Blender, "null");
		for (int i=0; i<3; ++i)
			m_RendererTechnique[RS_CompRayData_Back+i] = shader->E[i];
	}

	{
		CBlender_fluid_raycast	Blender;
		ref_shader				shader;
		shader.create(&Blender, "null");
		for (int i=0; i<5; ++i)
			m_RendererTechnique[RS_QuadEdgeDetect+i] = shader->E[i];
	}	
}

void dx103DFluidRenderer::DestroyShaders()
{
	for (int i=0; i<RS_NumShaders; ++i)
	{
		//	Release shader's element.
		m_RendererTechnique[i] = 0;
	}
}

void dx103DFluidRenderer::CreateGridBox () 
{
	VsInput vertices[] =
	{
		{ D3DXVECTOR3( 0, 0, 0 ) },
		{ D3DXVECTOR3( 0, 0, 1 ) },
		{ D3DXVECTOR3( 0, 1, 0 ) },
		{ D3DXVECTOR3( 0, 1, 1 ) },
		{ D3DXVECTOR3( 1, 0, 0 ) },
		{ D3DXVECTOR3( 1, 0, 1 ) },
		{ D3DXVECTOR3( 1, 1, 0 ) },
		{ D3DXVECTOR3( 1, 1, 1 ) },
	};
	m_iGridBoxVertNum = sizeof(vertices)/sizeof(vertices[0]);

	//D3D_BUFFER_DESC bd;
	//bd.Usage = D3D_USAGE_DEFAULT;
	//bd.ByteWidth = sizeof(vertices);
	//bd.BindFlags = D3Dxx_BIND_VERTEX_BUFFER;
	//bd.CPUAccessFlags = 0;
	//bd.MiscFlags = 0;
	//D3Dxx_SUBRESOURCE_DATA InitData;
	//InitData.pSysMem = vertices;
	//V_RETURN( m_pD3DDevice->CreateBuffer( &bd, &InitData, &pGridBoxVertexBuffer ) );

	CHK_DX(dx10BufferUtils::CreateVertexBuffer(&m_pGridBoxVertexBuffer, vertices, sizeof(vertices)));


	// Create index buffer
	//DWORD indices[] =
	//DWORD indices[] =
	u16 indices[] =
	{
		0, 4, 1, 1, 4, 5,
		0, 1, 2, 2, 1, 3,
		4, 6, 5, 6, 7, 5,
		2, 3, 6, 3, 7, 6,
		1, 5, 3, 3, 5, 7,
		0, 2, 4, 2, 6, 4
	};
	m_iGridBoxFaceNum = (sizeof(indices)/sizeof(indices[0]))/3;

	//bd.Usage = D3D_USAGE_DEFAULT;
	//bd.ByteWidth = sizeof(indices);
	//bd.BindFlags = D3Dxx_BIND_INDEX_BUFFER;
	//bd.CPUAccessFlags = 0;
	//bd.MiscFlags = 0;
	//InitData.pSysMem = indices;
	//V_RETURN( m_pD3DDevice->CreateBuffer( &bd, &InitData, &pGridBoxIndexBuffer ) );

	CHK_DX(dx10BufferUtils::CreateIndexBuffer	(&m_pGridBoxIndexBuffer, indices, sizeof(indices)));
	HW.stats_manager.increment_stats				(sizeof(indices), enum_stats_buffer_type_index, D3DPOOL_MANAGED);

	// Define the input layout
	//D3Dxx_INPUT_ELEMENT_DESC layout[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3Dxx_INPUT_PER_VERTEX_DATA, 0 },  
	//};
	//UINT numElements = sizeof(layout)/sizeof(layout[0]);
	static D3DVERTEXELEMENT9 layout[] = 
	{
		{ 0, 0,		D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_POSITION,		0 },
		D3DDECL_END()
	};

	// Create the input layout
	//D3Dxx_PASS_DESC PassDesc;
	//pTechnique->GetPassByName("CompRayData_Back")->GetDesc( &PassDesc );
	//V_RETURN( m_pD3DDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pGridBoxLayout ) );

	m_GeomGridBox.create(layout, m_pGridBoxVertexBuffer, m_pGridBoxIndexBuffer);
}

void dx103DFluidRenderer::CreateScreenQuad() 
{
	// Create our quad input layout
	//const D3Dxx_INPUT_ELEMENT_DESC quadlayout[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3Dxx_INPUT_PER_VERTEX_DATA, 0 },
	//};
	//UINT numElements = sizeof(quadlayout)/sizeof(quadlayout[0]);

	static D3DVERTEXELEMENT9 quadlayout[] = 
	{
		{ 0, 0,		D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_POSITION,		0 },
		D3DDECL_END()
	};

	// Create the input layout
	//D3Dxx_PASS_DESC PassDesc;
	//V_RETURN(pTechnique->GetPassByName("QuadRaycast")->GetDesc( &PassDesc ));
	//V_RETURN( m_pD3DDevice->CreateInputLayout( quadlayout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pQuadLayout ) );

	// Create a screen quad for all render to texture operations
	VsInput svQuad[4];
	svQuad[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.0f );
	svQuad[1].pos = D3DXVECTOR3(1.0f, 1.0f, 0.0f );
	svQuad[2].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.0f );
	svQuad[3].pos = D3DXVECTOR3(1.0f, -1.0f, 0.0f );

	//D3D_BUFFER_DESC vbdesc =
	//{
	//	4*sizeof(VsInput),
	//	D3D_USAGE_DEFAULT,
	//	D3Dxx_BIND_VERTEX_BUFFER,
	//	0,
	//	0
	//};

	//D3Dxx_SUBRESOURCE_DATA InitData;
	//InitData.pSysMem = svQuad;
	//InitData.SysMemPitch = 0;
	//InitData.SysMemSlicePitch = 0;
	//V_RETURN( m_pD3DDevice->CreateBuffer( &vbdesc, &InitData, &pQuadVertexBuffer ) );

	CHK_DX(dx10BufferUtils::CreateVertexBuffer(&m_pQuadVertexBuffer, svQuad, sizeof(svQuad)));
	m_GeomQuadVertex.create(quadlayout, m_pQuadVertexBuffer, 0);
}

void dx103DFluidRenderer::CreateJitterTexture()
{
	BYTE data[256 * 256];
	for (int i = 0; i < 256 * 256; i++)
	{
		data[i] = (unsigned char) (rand()/float(RAND_MAX)*256);
	}

	D3D_TEXTURE2D_DESC desc;
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	//desc.Format = DXGI_FORMAT_R8_TYPELESS;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	//desc.Usage = D3D_USAGE_IMMUTABLE;
#ifdef USE_DX11
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
#else
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
#endif

	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D_SUBRESOURCE_DATA dataDesc;
	dataDesc.pSysMem = data;
	dataDesc.SysMemPitch = 256;

	ID3DTexture2D* NoiseTexture = NULL;
	//ID3DxxShaderResourceView* JitterTextureSRV = NULL;

	CHK_DX( HW.pDevice->CreateTexture2D(&desc, &dataDesc, &NoiseTexture));

	//( m_pD3DDevice->CreateTexture2D(&desc, &dataDesc, &NoiseTexture) );

	// Create the shader resource view for jittering
	//D3Dxx_SHADER_RESOURCE_VIEW_DESC descSRV;

	//ZeroMemory( &descSRV, sizeof(descSRV) );
	//descSRV.Format = DXGI_FORMAT_R8_UNORM;
	//descSRV.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	//descSRV.Texture2D.MipLevels = 1;
	//descSRV.Texture2D.MostDetailedMip = 0;

	//V( m_pD3DDevice->CreateShaderResourceView( NoiseTexture, &descSRV, &JitterTextureSRV ) );
	//pEffect->GetVariableByName("jitterTex")->AsShaderResource() -> SetResource (JitterTextureSRV);

	m_JitterTexture = dxRenderDeviceRender::Instance().Resources->_CreateTexture("$user$NVjitterTex");
	m_JitterTexture->surface_set(NoiseTexture);


	_RELEASE(NoiseTexture);
	//SAFE_RELEASE(JitterTextureSRV);
}

namespace
{
	// cubic b-spline
	float bsW0(float a)
	{
		return (1.0f/6.0f * ( -(a*a*a) + (3.0f * a*a) - (3.0f * a) + 1.0f));
	}

	float bsW1(float a)
	{
		return (1.0f/6.0f * ( (3.0f * a*a*a) - (6.0f * a*a) + 4.0f ));
	}

	float bsW2(float a)
	{
		return (1.0f/6.0f * ( -(3.0f * a*a*a) + (3.0f * a*a) + (3.0f*a) + 1.0f));
	}

	float bsW3(float a)
	{
		return (1.0f/6.0f * a*a*a);
	}

	float g0(float a)
	{
		return (bsW0(a) + bsW1(a));
	}

	float g1(float a)
	{
		return (bsW2(a) + bsW3(a));
	}

	float h0texels(float a)
	{
		return (1.0f + a - (bsW1(a)/(bsW0(a)+bsW1(a))));
	}

	float h1texels(float a)
	{
		return (1.0f - a + (bsW3(a)/(bsW2(a)+bsW3(a))));
	}
}

void dx103DFluidRenderer::CreateHHGGTexture()
{
//	static const int iNumSamples = 256;
	static const int iNumSamples = 16;
//	static const int iNumSamples = 1;
	float data[4*iNumSamples];
	D3DXFLOAT16 converted[4*iNumSamples];

//	Fvector4 mmin;
//	Fvector4 mmax;

//	mmin.set(10000, 10000, 10000, 10000);
//	mmax.set(-10000, -10000, -10000, -10000);

	for (int i = 0; i < iNumSamples; i++)
	{
		float a = i / (float)(iNumSamples-1);
		data[4*i] = -h0texels(a);
		data[4*i+1] = h1texels(a);
		data[4*i+2] = 1.0f-g0(a);
		data[4*i+3] = g0(a);

//		for ( int j=0; j < 4; ++j )
//		{
//			mmin[j] = _min(mmin[j], data[4*i+j]);
//			mmax[j] = _max(mmax[j], data[4*i+j]);
//		}
	}

	//	Min value is -1
	//	Max value is +1

	D3DXFloat32To16Array( converted, data, 4*iNumSamples );

	D3D_TEXTURE1D_DESC desc;
	desc.Width = iNumSamples;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	//desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	//desc.Usage = D3D_USAGE_IMMUTABLE;
#ifdef USE_DX11
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
#else
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
#endif
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D_SUBRESOURCE_DATA dataDesc;
	//dataDesc.pSysMem = data;
	//dataDesc.SysMemPitch = sizeof(data);
	dataDesc.pSysMem = converted;
	dataDesc.SysMemPitch = sizeof(converted);

	ID3DTexture1D* HHGGTexture = NULL;

	CHK_DX( HW.pDevice->CreateTexture1D(&desc, &dataDesc, &HHGGTexture));

	m_HHGGTexture = dxRenderDeviceRender::Instance().Resources->_CreateTexture("$user$NVHHGGTex");
	m_HHGGTexture->surface_set(HHGGTexture);

	_RELEASE(HHGGTexture);
}

void dx103DFluidRenderer::SetScreenSize( int width, int height )
{
	CreateRayDataResources(width, height);
}

void dx103DFluidRenderer::CalculateRenderTextureSize(int screenWidth, int screenHeight)
{
	int maxProjectedSide = int(3.0 * _sqrt(3.0)*m_fMaxDim);
	int maxScreenDim = _max(screenWidth, screenHeight);

	float screenAspectRatio = ((float)screenWidth)/screenHeight;

	if( maxScreenDim > maxProjectedSide)
	{
		if(screenHeight > screenWidth)
		{
			m_iRenderTextureHeight = maxProjectedSide;
			m_iRenderTextureWidth = (int)(screenAspectRatio * maxProjectedSide);
		}
		else
		{
			m_iRenderTextureWidth = maxProjectedSide;
			m_iRenderTextureHeight = (int)((1.0f/screenAspectRatio) * maxProjectedSide);
		}
	}
	else
	{
		m_iRenderTextureWidth = screenWidth;
		m_iRenderTextureHeight = screenHeight;
	}
}

void dx103DFluidRenderer::CreateRayDataResources( int width, int height )
{

//	SAFE_RELEASE(pRayDataTex2D);
//	SAFE_RELEASE(pRayDataSRV);
//	SAFE_RELEASE(pRayDataRTV);
//	SAFE_RELEASE(pRayDataSmallTex2D);
//	SAFE_RELEASE(pRayDataSmallSRV);
//	SAFE_RELEASE(pRayDataSmallRTV);
//	SAFE_RELEASE(pRayCastTex2D);
//	SAFE_RELEASE(pRayCastSRV);
//	SAFE_RELEASE(pRayCastRTV);
//	SAFE_RELEASE(pEdgeTex2D);
//	SAFE_RELEASE(pEdgeSRV);
//	SAFE_RELEASE(pEdgeRTV);

	
	// find a good resolution for raycasting purposes
	CalculateRenderTextureSize(width, height);
/*
	DXGI_FORMAT volumeDataFmt = DXGI_FORMAT_R32G32B32A32_FLOAT;

	D3D_TEXTURE2D_DESC desc;
	desc.ArraySize = 1;
	desc.BindFlags = D3D_BIND_SHADER_RESOURCE | D3Dxx_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.Width = width;
	desc.Height = height;
	desc.Format = volumeDataFmt;
	V_RETURN(m_pD3DDevice->CreateTexture2D(&desc,NULL,&pRayDataTex2D));

	desc.Width = m_iRenderTextureWidth;
	desc.Height = m_iRenderTextureHeight;
	V_RETURN(m_pD3DDevice->CreateTexture2D(&desc,NULL,&pRayDataSmallTex2D));
	V_RETURN(m_pD3DDevice->CreateTexture2D(&desc,NULL,&pRayCastTex2D));

	desc.Format = DXGI_FORMAT_R32_FLOAT;
	V_RETURN(m_pD3DDevice->CreateTexture2D(&desc,NULL,&pEdgeTex2D));

	D3Dxx_RENDER_TARGET_VIEW_DESC DescRT;
	DescRT.Format = volumeDataFmt;
	DescRT.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2D;
	DescRT.Texture2D.MipSlice = 0;
	V_RETURN( m_pD3DDevice->CreateRenderTargetView(pRayDataTex2D, &DescRT, &pRayDataRTV));
	V_RETURN( m_pD3DDevice->CreateRenderTargetView(pRayDataSmallTex2D, &DescRT, &pRayDataSmallRTV));
	V_RETURN( m_pD3DDevice->CreateRenderTargetView(pRayCastTex2D, &DescRT, &pRayCastRTV));
	DescRT.Format = DXGI_FORMAT_R32_FLOAT;
	V_RETURN( m_pD3DDevice->CreateRenderTargetView(pEdgeTex2D, &DescRT, &pEdgeRTV));

	D3Dxx_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
	SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Format = volumeDataFmt;
	V_RETURN(m_pD3DDevice->CreateShaderResourceView(pRayDataTex2D, &SRVDesc, &pRayDataSRV));
	V_RETURN(m_pD3DDevice->CreateShaderResourceView(pRayDataSmallTex2D, &SRVDesc, &pRayDataSmallSRV));
	V_RETURN(m_pD3DDevice->CreateShaderResourceView(pRayCastTex2D, &SRVDesc, &pRayCastSRV));
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	V_RETURN(m_pD3DDevice->CreateShaderResourceView(pEdgeTex2D, &SRVDesc, &pEdgeSRV));


	pRayDataVar = pEffect->GetVariableByName("rayDataTex")->AsShaderResource();
	pRayDataSmallVar = pEffect->GetVariableByName("rayDataTexSmall")->AsShaderResource();
	pRayCastVar = pEffect->GetVariableByName("rayCastTex")->AsShaderResource();
	pEdgeVar    = pEffect->GetVariableByName("edgeTex")->AsShaderResource();
	*/

	RT[0] = 0;
	RT[0].create( m_pRTNames[0], width, height, RTFormats[0]);

	for (int i=1; i<RRT_NumRT; ++i)
	{
		RT[i] = 0;
		RT[i].create( m_pRTNames[i], m_iRenderTextureWidth, m_iRenderTextureHeight, RTFormats[i]);
	}
}

void dx103DFluidRenderer::Draw(const dx103DFluidData &FluidData)
{
	//	We don't need ZB anyway
	RCache.set_ZB(0);

	const dx103DFluidData::Settings &VolumeSettings = FluidData.GetSettings();
	const bool bRenderFire = (VolumeSettings.m_SimulationType == dx103DFluidData::ST_FIRE);

	FogLighting  LightData;

	CalculateLighting(FluidData, LightData);


	const Fmatrix &transform = FluidData.GetTransform();

	RCache.set_xform_world( transform );

	//pColorTexVar->SetResource(pSourceTexSRV);

	//	Set shader element to set up all necessary constants to constant buffer
	//	If you change constant buffer layout make sure this hack works ok.
	RCache.set_Element(m_RendererTechnique[RS_CompRayData_Back]);

	// Set some variables required by the shaders:
	//=========================================================================

	// The near and far planes are used to unproject the scene's z-buffer values
	//pZNearVar->SetFloat(g_zNear);
	RCache.set_c(strZNear, VIEWPORT_NEAR);
	//pZFarVar->SetFloat(g_zFar);
	RCache.set_c(strZFar, g_pGamePersistent->Environment().CurrentEnv->far_plane);

	//D3DXMATRIX worldView = g_gridWorld * g_View;
	D3DXMATRIX gridWorld;
	//D3DXMatrixTranspose(&gridWorld, (D3DXMATRIX*)&transform);
	gridWorld = *(D3DXMATRIX*)&transform;
	D3DXMATRIX View;
	//D3DXMatrixTranspose(&View, (D3DXMATRIX*)&RCache.xforms.m_v);
	View = *(D3DXMATRIX*)&RCache.xforms.m_v;
	D3DXMATRIX WorldView = gridWorld * View;

	//	Modified later
	//Fmatrix	WorldView = RCache.xforms.m_wv;
	//RCache.set_xform_world( transform );

	// The length of one of the axis of the worldView matrix is the length of longest side of the box
	//  in view space. This is used to convert the length of a ray from view space to grid space.
	//D3DXVECTOR3 worldXaxis = D3DXVECTOR3(worldView._11, worldView._12, worldView._13);
	D3DXVECTOR3 worldXaxis = D3DXVECTOR3(WorldView._11, WorldView._12, WorldView._13);
	float worldScale = D3DXVec3Length(&worldXaxis);
	//pGridScaleFactorVar->SetFloat( worldScale );
	RCache.set_c(strGridScaleFactor, worldScale);

	// We prepend the current world matrix with this other matrix which adds an offset (-0.5, -0.5, -0.5)
	//  and scale factors to account for unequal number of voxels on different sides of the volume box. 
	// This is because we want to preserve the aspect ratio of the original simulation grid when 
	//  raytracing through it.
	//worldView = m_gridMatrix * worldView;
	WorldView = m_gridMatrix * WorldView;
	//WorldView.mulB_44(m_gridMatrix);

//	Fmatrix temp;
//	temp = transform;
//	temp.mulB_44(m_gridMatrix);

//	RCache.set_xform_world( temp );
//	return;
	

	// worldViewProjection is used to transform the volume box to screen space
	//D3DXMATRIX WorldViewProjection;
	D3DXMATRIX WorldViewProjection;
	//Fmatrix WorldViewProjection;
	//worldViewProjection = worldView * g_Projection;
	D3DXMATRIX Projection;
	//D3DXMatrixTranspose(&Projection, (D3DXMATRIX*)&RCache.xforms.m_p);
	Projection = *(D3DXMATRIX*)&RCache.xforms.m_p;
	WorldViewProjection = WorldView * Projection;
	//WorldViewProjection.mul(RCache.xforms.m_p, WorldView);
	//pWorldViewProjectionVar->SetMatrix( (float*)&worldViewProjection );
	//	NVidia used different matrix orientation
	//WorldViewProjection.transpose();
	//RCache.set_c(strWorldViewProjection, WorldViewProjection);
	RCache.set_c(strWorldViewProjection, *(Fmatrix*)&WorldViewProjection);
	//WorldViewProjection.transpose();

	// invWorldViewProjection is used to transform positions in the "near" plane into grid space
	//D3DXMATRIX invWorldViewProjection;
	D3DXMATRIX InvWorldViewProjection;
	//Fmatrix InvWorldViewProjection;
	//WorldViewProjection.transpose();
	D3DXMatrixInverse((D3DXMATRIX*)&InvWorldViewProjection, NULL, (D3DXMATRIX*)&WorldViewProjection);
	//WorldViewProjection.transpose();
	//pInvWorldViewProjectionVar->SetMatrix((float*)&invWorldViewProjection);
	//InvWorldViewProjection.transpose();
	//RCache.set_c(strInvWorldViewProjection, InvWorldViewProjection);
	RCache.set_c(strInvWorldViewProjection, *(Fmatrix*)&InvWorldViewProjection);
	//InvWorldViewProjection.transpose();

	// Compute the inverse of the worldView matrix 
	//D3DXMATRIX worldViewInv;
	D3DXMATRIX WorldViewInv;
	//Fmatrix WorldViewInv;
	D3DXMatrixInverse((D3DXMATRIX*)&WorldViewInv, NULL, (D3DXMATRIX*)&WorldView);
	// Compute the eye's position in "grid space" (the 0-1 texture coordinate cube)
	//D3DXVECTOR4 eyeInGridSpace;
	//D3DXVECTOR3 origin(0,0,0);
	D3DXVECTOR4 EyeInGridSpace;
	D3DXVECTOR3 Origin(0,0,0);
	//Fvector4 EyeInGridSpace;
	//Fvector3 Origin = Fvector3().set(0,0,0);
	//WorldViewInv.transpose();
	D3DXVec3Transform((D3DXVECTOR4*)&EyeInGridSpace, (D3DXVECTOR3*)&Origin, (D3DXMATRIX*)&WorldViewInv);
	//WorldViewInv.transpose();
	//pEyeOnGridVar->SetFloatVector((float*)&eyeInGridSpace);
	RCache.set_c(strEyeOnGrid, *(Fvector4*)&EyeInGridSpace);

	float color[4] = {0, 0, 0, 0 };


	// Ray cast and render to a temporary buffer
	//=========================================================================

	// Partial init of viewport struct used below
	//D3Dxx_VIEWPORT rtViewport;
	//rtViewport.TopLeftX = 0;
	//rtViewport.TopLeftY = 0;
	//rtViewport.MinDepth = 0;
	//rtViewport.MaxDepth = 1;


	// Compute the ray data required by the raycasting pass below.
	//  This function will render to a buffer of float4 vectors, where
	//  xyz is starting position of the ray in grid space
	//  w is the length of the ray in view space
	ComputeRayData();


	// Do edge detection on this image to find any 
	//  problematic areas where we need to raycast at higher resolution
	ComputeEdgeTexture();


	// Raycast into the temporary render target: 
	//  raycasting is done at the smaller resolution, using a fullscreen quad
	//m_pD3DDevice->ClearRenderTargetView( pRayCastRTV, color );
	HW.pContext->ClearRenderTargetView( RT[RRT_RayCastTex]->pRT, color );
	//m_pD3DDevice->OMSetRenderTargets( 1, &pRayCastRTV , NULL ); 
	CRenderTarget* pTarget = RImplementation.Target;
	pTarget->u_setrt(RT[RRT_RayCastTex],0,0,0);		// LDR RT

	//rtViewport.Width = renderTextureWidth;
	//rtViewport.Height = renderTextureHeight;
	//m_pD3DDevice->RSSetViewports(1,&rtViewport);
	RImplementation.rmNormal();

	//pTechnique->GetPassByName("QuadRaycast")->Apply(0);
	if (bRenderFire)
		RCache.set_Element(m_RendererTechnique[RS_QuadRaycastFire]);
	else
		RCache.set_Element(m_RendererTechnique[RS_QuadRaycastFog]);

	//pRTWidthVar->SetFloat((float)renderTextureWidth);
	RCache.set_c(strRTWidth, (float)m_iRenderTextureWidth);
	//pRTHeightVar->SetFloat((float)renderTextureHeight);
	RCache.set_c(strRTHeight, (float)m_iRenderTextureHeight);

	//pRayDataSmallVar->SetResource(pRayDataSmallSRV);

	DrawScreenQuad();


	// Render to the back buffer sampling from the raycast texture that we just created
	//  If and edge was detected at the current pixel we will raycast again to avoid
	//  smoke aliasing artifacts at scene edges
	//ID3DxxRenderTargetView* pRTV = DXUTGetD3DxxRenderTargetView();
	//ID3DxxDepthStencilView* pDSV = DXUTGetD3DxxDepthStencilView();
	//m_pD3DDevice->OMSetRenderTargets( 1, &pRTV , pDSV ); 
	//	Restore render state
	if( !RImplementation.o.dx10_msaa )
		pTarget->u_setrt( pTarget->rt_Generic_0,0,0,HW.pBaseZB);		// LDR RT
	else
		pTarget->u_setrt( pTarget->rt_Generic_0_r,0,0,pTarget->rt_MSAADepth->pZRT);		// LDR RT

	if (bRenderFire)
		RCache.set_Element(m_RendererTechnique[RS_QuadRaycastCopyFire]);
	else
		RCache.set_Element(m_RendererTechnique[RS_QuadRaycastCopyFog]);

	//rtViewport.Width = g_Width;
	//rtViewport.Height = g_Height;
	//m_pD3DDevice->RSSetViewports(1,&rtViewport);
	RImplementation.rmNormal();

	//pRTWidthVar->SetFloat((float)g_Width);
	RCache.set_c(strRTWidth, (float)Device.dwWidth);
	//pRTHeightVar->SetFloat((float)g_Height);
	RCache.set_c(strRTHeight, (float)Device.dwHeight);

	RCache.set_c(strDiffuseLight, LightData.m_vLightIntencity.x, LightData.m_vLightIntencity.y, LightData.m_vLightIntencity.z, 1.0f);

	//pRayCastVar->SetResource(pRayCastSRV);
	//pEdgeVar->SetResource(pEdgeSRV);

	//pTechnique->GetPassByName("QuadRaycastCopy")->Apply(0);
	DrawScreenQuad();
}

void dx103DFluidRenderer::ComputeRayData()
{
	// Clear the color buffer to 0
	float blackColor[4] = {0, 0, 0, 0 };
	//m_pD3DDevice->ClearRenderTargetView(pRayDataRTV, blackColor);
	HW.pContext->ClearRenderTargetView( RT[RRT_RayDataTex]->pRT, blackColor );
	//m_pD3DDevice->OMSetRenderTargets(1, &pRayDataRTV, NULL);
	CRenderTarget* pTarget = RImplementation.Target;
	pTarget->u_setrt(RT[RRT_RayDataTex],0,0,0);		// LDR RT
	//pEffect->GetVariableByName("sceneDepthTex")->AsShaderResource()->SetResource(g_pSceneDepthSRV);
	RCache.set_Element(m_RendererTechnique[RS_CompRayData_Back]);

	// Setup viewport to match the window's backbuffer
	//D3Dxx_VIEWPORT rtViewport;
	//rtViewport.TopLeftX = 0;
	//rtViewport.TopLeftY = 0;
	//rtViewport.MinDepth = 0;
	//rtViewport.MaxDepth = 1;
	//rtViewport.Width = g_Width;
	//rtViewport.Height = g_Height;
	//m_pD3DDevice->RSSetViewports(1,&rtViewport);
	RImplementation.rmNormal();
	//pRTWidthVar->SetFloat((float)g_Width);
	RCache.set_c(strRTWidth, (float)Device.dwWidth);
	//pRTHeightVar->SetFloat((float)g_Height);
	RCache.set_c(strRTHeight, (float)Device.dwHeight);

	// Render volume back faces
	// We output xyz=(0,-1,0) and w=min(sceneDepth, boxDepth)
	//pTechnique->GetPassByName("CompRayData_Back")->Apply(0);
	DrawBox();

	// Render volume front faces using subtractive blending
	// We output xyz="position in grid space" and w=boxDepth,
	//  unless the pixel is occluded by the scene, in which case we output xyzw=(1,0,0,0)
	//m_pD3DDevice->OMSetRenderTargets(1, &pRayDataRTV, NULL);
	pTarget->u_setrt(RT[RRT_RayDataTex],0,0,0);		// LDR RT
	RCache.set_Element(m_RendererTechnique[RS_CompRayData_Front]);
	//pTechnique->GetPassByName("CompRayData_Front")->Apply(0);
	DrawBox();

}

void dx103DFluidRenderer::ComputeEdgeTexture()
{
	CRenderTarget* pTarget = RImplementation.Target;
	pTarget->u_setrt(RT[RRT_RayDataTexSmall],0,0,0);		// LDR RT
	RCache.set_Element(m_RendererTechnique[RS_QuadDownSampleRayDataTexture]);

	// First setup viewport to match the size of the destination low-res texture
	//D3Dxx_VIEWPORT rtViewport;
	//rtViewport.TopLeftX = 0;
	//rtViewport.TopLeftY = 0;
	//rtViewport.MinDepth = 0;
	//rtViewport.MaxDepth = 1;
	//rtViewport.Width = renderTextureWidth;
	//rtViewport.Height = renderTextureHeight;
	//m_pD3DDevice->RSSetViewports(1,&rtViewport);
	RImplementation.rmNormal();
	//pRTWidthVar->SetFloat((float)renderTextureWidth);
	RCache.set_c(strRTWidth, (float)m_iRenderTextureWidth);
	//pRTHeightVar->SetFloat((float)renderTextureHeight);
	RCache.set_c(strRTHeight, (float)m_iRenderTextureHeight);

	// Downsample the rayDataTexture to a new small texture, simply using point sample (no filtering)
	//m_pD3DDevice->OMSetRenderTargets( 1, &pRayDataSmallRTV , NULL ); 
	//pRayDataVar->SetResource(pRayDataSRV);
	//pTechnique->GetPassByName("QuadDownSampleRayDataTexture")->Apply(0);
	DrawScreenQuad();

	// Create an edge texture, performing edge detection on 'rayDataTexSmall'
	pTarget->u_setrt(RT[RRT_EdgeTex],0,0,0);		// LDR RT
	RCache.set_Element(m_RendererTechnique[RS_QuadEdgeDetect]);
	//m_pD3DDevice->OMSetRenderTargets( 1, &pEdgeRTV , NULL ); 
	//pRayDataSmallVar->SetResource(pRayDataSmallSRV);
	//pTechnique->GetPassByName("QuadEdgeDetect")->Apply(0);
	DrawScreenQuad();
}

void dx103DFluidRenderer::DrawScreenQuad()
{
	//UINT strides = sizeof(VsInput);
	//UINT offsets = 0;
	//m_pD3DDevice->IASetInputLayout( pQuadLayout );
	//m_pD3DDevice->IASetVertexBuffers( 0, 1, &pQuadVertexBuffer, &strides, &offsets );
	//m_pD3DDevice->IASetPrimitiveTopology( D3Dxx_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
	//m_pD3DDevice->Draw( 4, 0 );

	RCache.set_Geometry(m_GeomQuadVertex);
	RCache.Render( D3DPT_TRIANGLESTRIP, 0, 2);
}

void dx103DFluidRenderer::DrawBox()
{
//	UINT stride = sizeof( VsInput );
//	UINT offset = 0;
//	m_pD3DDevice->IASetVertexBuffers( 0, 1, &pGridBoxVertexBuffer, &stride, &offset );
//	m_pD3DDevice->IASetIndexBuffer( pGridBoxIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
//	m_pD3DDevice->IASetPrimitiveTopology( D3Dxx_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
//	m_pD3DDevice->IASetInputLayout(pGridBoxLayout);
//	m_pD3DDevice->DrawIndexed(36, 0, 0);

	RCache.set_Geometry(m_GeomGridBox);
	RCache.Render( D3DPT_TRIANGLELIST, 0, 0, m_iGridBoxVertNum ,0, m_iGridBoxFaceNum);
}

void dx103DFluidRenderer::CalculateLighting(const dx103DFluidData &FluidData, FogLighting  &LightData)
{
	m_lstRenderables.clear_not_free();

	LightData.Reset();

	const dx103DFluidData::Settings &VolumeSettings = FluidData.GetSettings();

	Fvector4 hemi_color = g_pGamePersistent->Environment().CurrentEnv->hemi_color;
	//hemi_color.mul(0.2f);
	hemi_color.mul(VolumeSettings.m_fHemi);
	LightData.m_vLightIntencity.set(hemi_color.x, hemi_color.y, hemi_color.z);
	LightData.m_vLightIntencity.add(g_pGamePersistent->Environment().CurrentEnv->ambient);

	const Fmatrix &Transform = FluidData.GetTransform();

	Fbox	box;
	box.min = Fvector3().set(-0.5f, -0.5f, -0.5f);
	box.max = Fvector3().set( 0.5f,  0.5f,  0.5f);
	box.xform(Transform);
	Fvector3	center;
	Fvector3	size;
	box.getcenter(center);
	box.getradius(size);


	// Traverse object database
	g_SpatialSpace->q_box
		(
		m_lstRenderables,
		0, //ISpatial_DB::O_ORDERED,
		STYPE_LIGHTSOURCE,
		center,
		size
		);

	u32 iNumRenderables = m_lstRenderables.size();
	// Determine visibility for dynamic part of scene
	for (u32 i=0; i<iNumRenderables; ++i)
	{
		ISpatial*	spatial		= m_lstRenderables[i];

		// Light
		light*	pLight = (light*) spatial->dcast_Light();
		VERIFY(pLight);

		if (pLight->flags.bStatic) continue;

		float	d	=	pLight->position.distance_to(Transform.c);

		float	R				= pLight->range + _max( size.x, _max( size.y, size.z ) );
		if ( d >= R )
			continue;

		Fvector3	LightIntencity;

		LightIntencity.set(pLight->color.r, pLight->color.g, pLight->color.b);

		//LightIntencity.mul(0.5f);

		//if (!pLight->flags.bStatic)
		//	LightIntencity.mul(0.5f);

		float	r	=	pLight->range;
		float	a	=	clampr(1.f - d/(r+EPS),0.f,1.f)*(pLight->flags.bStatic?1.f:2.f);

		LightIntencity.mul(a);

		LightData.m_vLightIntencity.add(LightIntencity);
	}

	//LightData.m_vLightIntencity.set( 1.0f, 0.5f, 0.0f);
	//LightData.m_vLightIntencity.set( 1.0f, 1.0f, 1.0f);
}