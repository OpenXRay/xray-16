#include "pch.h"
ENGINE_API extern  Fvector2 g_current_font_scale;

XRayFontRender::XRayFontRender() /*: m_index_count(0), m_vertex_count(0)*/
{
}

XRayFontRender::~XRayFontRender()
{
}

void XRayFontRender::Initialize(LPCSTR cShader, LPCSTR cTexture)
{
	/*GXRayRenderResource->CreateShader(cShader, m_shader);
	m_shader.SetTextureToPixel(0, cTexture);*/
}

void XRayFontRender::OnRender(CGameFont & owner)
{
	

/*	if (!(owner.uFlags&CGameFont::fsValid)) {


		if (Blender.E[0].Textures[0])
		{
			auto size = Blender.E[0].Textures[0]->GetSize();
			owner.vTS.set((int)size.x, (int)size.y);
			owner.fTCHeight = owner.fHeight / float(owner.vTS.y);
			owner.uFlags |= CGameFont::fsValid;
		}
	
	}

	for (u32 i = 0; i < owner.strings.size(); ) {
		// calculate first-fit
		int		count = 1;

		u32 length = owner.smart_strlen(owner.strings[i].string);

		while ((i + count) < owner.strings.size()) {
			int L = owner.smart_strlen(owner.strings[i + count].string);

			if ((L + length) < MAX_MB_CHARS) {
				count++;
				length += L;
			}
			else		break;
		}

		if (m_vertex_buffer_current == m_vertex_buffers.end())
		{
			BearFactoryPointer<BearRHI::BearRHIVertexBuffer> vertex_buffer = BearRenderInterface::CreateVertexBuffer();
			vertex_buffer->Create(GResourcesManager->GetStride(FVF::F_TL),bear_recommended_size( length * 6), true);
			m_vertex_buffers.push_back(vertex_buffer);
			m_vertex_buffer_current = --m_vertex_buffers.end();

		}
		else
		{
			if ((*m_vertex_buffer_current)->GetCount() < length * 6)
			{
				(*m_vertex_buffer_current)->Create(GResourcesManager->GetStride(FVF::F_TL), bear_recommended_size(length * 6), true);
			}
		}

		

		FVF::TL* v = (FVF::TL*)	(*m_vertex_buffer_current)->Lock();

		FVF::TL* start = v;

		// fill vertices
		u32 last = i + count;
		for (; i < last; i++) {
			CGameFont::String		&PS = owner.strings[i];
			wide_char wsStr[MAX_MB_CHARS];

			int	len = owner.IsMultibyte() ?
				mbhMulti2Wide(wsStr, NULL, MAX_MB_CHARS, PS.string) :
				xr_strlen(PS.string);

			if (len) {
				float	X = float(XrMath::iFloor(PS.x));
				float	Y = float(XrMath::iFloor(PS.y));
				float	S = PS.height*g_current_font_scale.y;
				float	Y2 = Y + S;
				float fSize = 0;

				if (PS.align)
					fSize = owner.IsMultibyte() ? owner.SizeOf_(wsStr) : owner.SizeOf_(PS.string);

				switch (PS.align)
				{
				case CGameFont::alCenter:
					X -= (XrMath::iFloor(fSize * 0.5f)) * g_current_font_scale.x;
					break;
				case CGameFont::alRight:
					X -= XrMath::iFloor(fSize);
					break;
				}

				u32	clr, clr2;
				clr2 = clr = PS.c;
				if (owner.uFlags&CGameFont::fsGradient) {
					u32	_R =XrColor::color_get_R(clr) / 2;
					u32	_G =XrColor::color_get_G(clr) / 2;
					u32	_B =XrColor::color_get_B(clr) / 2;
					u32	_A =XrColor::color_get_A(clr);
					clr2 =XrColor::color_rgba(_R, _G, _B, _A);
				}

				X -= 0.5f;
				Y -= 0.5f;
				Y2 -= 0.5f;

				float	tu, tv;
				for (int j = 0; j < len; j++)
				{
					Fvector l;

					l = owner.IsMultibyte() ? owner.GetCharTC(wsStr[1 + j]) : owner.GetCharTC((u16)(u8)PS.string[j]);

					float scw = l.z * g_current_font_scale.x;

					float fTCWidth = l.z / owner.vTS.x;

					if (!XrMath::fis_zero(l.z))
					{
						//						tu			= ( l.x / owner.vTS.x ) + ( 0.5f / owner.vTS.x );
						//						tv			= ( l.y / owner.vTS.y ) + ( 0.5f / owner.vTS.y );
						tu = (l.x / owner.vTS.x);
						tv = (l.y / owner.vTS.y);
#ifndef	USE_DX10
						//	Make half pixel offset for 1 to 1 mapping
						tu += (0.5f / owner.vTS.x);
						tv += (0.5f / owner.vTS.y);
#endif	//	USE_DX10

						v->set(X, Y2, clr2, tu, tv + owner.fTCHeight);						v++;
						v->set(X, Y, clr, tu, tv);									v++;
						v->set(X + scw, Y2, clr2, tu + fTCWidth, tv + owner.fTCHeight);		v++;
						v->set(X, Y, clr, tu, tv);									v++;
						v->set(X + scw, Y, clr, tu + fTCWidth, tv);					v++;
						v->set(X + scw, Y2, clr2, tu + fTCWidth, tv + owner.fTCHeight);		v++;
					}
					X += scw * owner.vInterval.x;
					if (owner.IsMultibyte()) {
						X -= 2;
						if (IsNeedSpaceCharacter(wsStr[1 + j]))
							X += owner.fXStep;
					}
				}
			}
		}

		// Unlock and draw
		u32 vCount = (u32)(v - start);
		(*m_vertex_buffer_current)->Unlock();

		if (vCount)
		{
			if (!Blender.E[0].Set(HW->Context)) { return; }

			HW->Context->SetVertexBuffer(*m_vertex_buffer_current);
			HW->Context->Draw(vCount);
			m_vertex_buffer_current++;
		}
		
	}*/
}

void XRayFontRender::Flush()
{
	/*m_vertex_buffer_current = m_vertex_buffers.begin();*/
}
