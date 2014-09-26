#include "stdafx.h"
#include "dx10ConstantBuffer.h"

#include "dx10BufferUtils.h"
#include "../xrRender/dxRenderDeviceRender.h"

dx10ConstantBuffer::~dx10ConstantBuffer()
{
	DEV->_DeleteConstantBuffer(this);
//	Flush();
	_RELEASE(m_pBuffer);
	xr_free(m_pBufferData);
}

dx10ConstantBuffer::dx10ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable)
	: m_bChanged(true)
{
	D3D_SHADER_BUFFER_DESC Desc;

	CHK_DX(pTable->GetDesc(&Desc));

	m_strBufferName._set(Desc.Name);
	m_eBufferType = Desc.Type;
	m_uiBufferSize = Desc.Size;

	//	Fill member list with variable descriptions
	m_MembersList.resize(Desc.Variables);
	m_MembersNames.resize(Desc.Variables);
	for (u32 i=0; i<Desc.Variables; ++i)
	{
		ID3DShaderReflectionVariable* pVar;
		ID3DShaderReflectionType*		pType;

		D3D_SHADER_VARIABLE_DESC		var_desc;

		pVar = pTable->GetVariableByIndex(i);
		VERIFY(pVar);
		pType = pVar->GetType();
		VERIFY(pType);
		pType->GetDesc(&m_MembersList[i]);
		//	Buffers with the same layout can contain totally different members
		CHK_DX(pVar->GetDesc(&var_desc));
		m_MembersNames[i] = var_desc.Name;
	}

	m_uiMembersCRC = crc32( &m_MembersList[0], Desc.Variables*sizeof(m_MembersList[0]));

	R_CHK(dx10BufferUtils::CreateConstantBuffer(&m_pBuffer, Desc.Size));
	VERIFY(m_pBuffer);
	m_pBufferData = xr_malloc(Desc.Size);
	VERIFY(m_pBufferData);
}

bool dx10ConstantBuffer::Similar(dx10ConstantBuffer &_in)
{
	if ( m_strBufferName._get() != _in.m_strBufferName._get() )
		return false;

	if ( m_eBufferType != _in.m_eBufferType )
		return false;

	if ( m_uiMembersCRC != _in.m_uiMembersCRC )
		return false;

	if ( m_MembersList.size() != _in.m_MembersList.size() )
		return false;

	if ( memcmp(&m_MembersList[0], &_in.m_MembersList[0], m_MembersList.size()*sizeof(m_MembersList[0])) )
		return false;

	VERIFY(m_MembersNames.size() == _in.m_MembersNames.size());

	int iMemberNum = m_MembersNames.size();
	for ( int i=0; i<iMemberNum; ++i)
	{
		if (m_MembersNames[i].c_str()!=_in.m_MembersNames[i].c_str())
			return false;
	}

	return true;
}

void dx10ConstantBuffer::Flush()
{
	if (m_bChanged)
	{
		void	*pData;
#ifdef USE_DX11
		D3D11_MAPPED_SUBRESOURCE	pSubRes;
		CHK_DX(HW.pContext->Map(m_pBuffer, 0, D3D_MAP_WRITE_DISCARD, 0, &pSubRes));
		pData = pSubRes.pData;
#else
		CHK_DX(m_pBuffer->Map(D3D_MAP_WRITE_DISCARD, 0, &pData));
#endif
		VERIFY(pData);
		VERIFY(m_pBufferData);
		CopyMemory(pData, m_pBufferData, m_uiBufferSize);
#ifdef USE_DX11
		HW.pContext->Unmap(m_pBuffer, 0);
#else
		m_pBuffer->Unmap();
#endif
		m_bChanged = false;
	}
}