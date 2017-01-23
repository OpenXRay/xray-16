#ifndef	dx10ConstantBuffer_included
#define	dx10ConstantBuffer_included
#pragma once

struct	R_constant;
struct	R_constant_load;

class dx10ConstantBuffer : public xr_resource_named
{
public:
	dx10ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable);
	~dx10ConstantBuffer();

	bool			Similar(dx10ConstantBuffer &_in);
	ID3DBuffer*		GetBuffer() { return m_pBuffer; }

	void			Flush();

	//	Set copy data into constant buffer
	//	Plain buffer member
	void			set(R_constant* C, R_constant_load& L, const Fmatrix& A);
	void			set(R_constant* C, R_constant_load& L, const Fvector4& A);
	void			set(R_constant* C, R_constant_load& L, float A);
	void			set(R_constant* C, R_constant_load& L, int A);
	//	Array buffer member
	void			seta(R_constant* C, R_constant_load& L, u32 e, const Fmatrix& A);
	void			seta(R_constant* C, R_constant_load& L, u32 e, const Fvector4& A);

	void*			AccessDirect(R_constant_load& L, u32 DataSize);

private:
	Fvector4*		Access(u16	offset);

private:
	shared_str							m_strBufferName;
	D3D_CBUFFER_TYPE					m_eBufferType;

	//	Buffer data description
	u32									m_uiMembersCRC;
	xr_vector<D3D_SHADER_TYPE_DESC>	m_MembersList;
	xr_vector<shared_str>				m_MembersNames;

	ID3DBuffer*							m_pBuffer;
	u32									m_uiBufferSize;	//	Cache buffer size for debug validation
	void*								m_pBufferData;
	bool								m_bChanged;

	static const u32					lineSize = sizeof(Fvector4);

	//	Never try to copy objects of this class due to the pointer and autoptr members
	dx10ConstantBuffer( const dx10ConstantBuffer&);
	dx10ConstantBuffer& operator=(dx10ConstantBuffer&);
};

typedef	resptr_core<dx10ConstantBuffer,resptr_base<dx10ConstantBuffer> > ref_cbuffer;

#endif	//	dx10ConstantBuffer_included