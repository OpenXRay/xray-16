#pragma once

#ifdef USE_OGL

namespace glBufferUtils
{
	void	CreateVertexBuffer(GLuint* pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
	void	CreateIndexBuffer(GLuint* pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
	GLsizei GetFVFVertexSize(u32 FVF);
	GLsizei GetDeclVertexSize(const D3DVERTEXELEMENT9* decl);
	u32		GetDeclLength(const D3DVERTEXELEMENT9* decl);
	void	ConvertVertexDeclaration(u32 FVF, SDeclaration* decl);
	void	ConvertVertexDeclaration(const D3DVERTEXELEMENT9* dxdecl, SDeclaration* decl);
};

#endif // USE_OGL
