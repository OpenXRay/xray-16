#include "stdafx.h"
#include "glApplicationRender.h"
#include "../../xrEngine/x_ray.h"

#include "../../xrEngine/GameFont.h"

void glApplicationRender::Copy(IApplicationRender &_in)
{
	*this = *(glApplicationRender*)&_in;
}

void glApplicationRender::LoadBegin()
{
	ll_hGeom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
	sh_progress.create("hud\\default", "ui\\ui_load");
	hLevelLogo_Add.create("hud\\default", "ui\\ui_ingame2_back_add2_w.tga");

	ll_hGeom2.create(FVF::F_TL, RCache.Vertex.Buffer(), NULL);
}

void glApplicationRender::destroy_loading_shaders()
{
	hLevelLogo.destroy();
	sh_progress.destroy();
	hLevelLogo_Add.destroy();
}

void glApplicationRender::setLevelLogo(LPCSTR pszLogoName)
{
	hLevelLogo.create("hud\\default", pszLogoName);
}

void glApplicationRender::KillHW()
{
	VERIFY(!"glApplicationRender::KillHW not implemented.");
}

u32 calc_progress_color(u32, u32, int, int);

void glApplicationRender::load_draw_internal(CApplication &owner)
{
	if (!sh_progress)
	{
		CHK_DX(HW.pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0));
		return;
	}

	float	_w = (float)Device.dwWidth;
	float	_h = (float)Device.dwHeight;
	bool	b_ws = (_w / _h) > 1.34f;
	bool	b_16x9 = b_ws && ((_w / _h)>1.77f);
	float	ws_k = (b_16x9) ? 0.75f : 0.8333f;	//16:9 or 16:10
	float	ws_w = b_ws ? (b_16x9 ? 171.0f : 102.6f) : 0.0f;

	float bw = 1024.0f;
	float bh = 768.0f;
	Fvector2					k;
	k.set(_w / bw, _h / bh);

	Fvector2					tsz;
	tsz.set(1024, 1024);
	Frect						back_tex_coords;
	Frect						back_coords;
	Fvector2					back_size;
	Fvector2					back_tex_size;

	//background picture
	static float offs = -0.5f;

	Fvector2					back_offset;
	if (b_ws)
		back_offset.set(ws_w*ws_k, 0.0f); //ws_w == 171
	else
		back_offset.set(0.0f, 0.0f);


	back_tex_size.set(1024, 768);
	back_size.set(1024, 768);
	if (b_ws)
		back_size.x *= ws_k; //ws

	back_tex_coords.lt.set(0, 0);
	back_tex_coords.rb.add(back_tex_coords.lt, back_tex_size);

	back_coords.lt.set(offs, offs);
	back_coords.lt.add(back_offset);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);
	draw_face(sh_progress, back_coords, back_tex_coords, tsz);

	if (b_ws) //draw additional frames (left&right)
	{
		//left
		back_size.set(ws_w*ws_k, 768.0f);

		if (b_16x9)
		{
			back_tex_coords.lt.set(682, 0);
			back_tex_coords.rb.set(850, 768);
		}
		else
		{
			back_tex_coords.lt.set(748, 0);
			back_tex_coords.rb.set(850, 768);
		}
		back_coords.lt.set(offs, offs);
		back_coords.rb.add(back_coords.lt, back_size);
		back_coords.lt.mul(k);
		back_coords.rb.mul(k);

		draw_face(hLevelLogo_Add, back_coords, back_tex_coords, tsz);

		//right
		if (b_16x9)
		{
			back_tex_coords.lt.set(850, 0);
			back_tex_coords.rb.set(1018, 768);
		}
		else
		{
			back_tex_coords.lt.set(850, 0);
			back_tex_coords.rb.set(952, 768);
		}

		back_coords.lt.set(1024.0f - back_size.x + offs, offs);
		back_coords.rb.add(back_coords.lt, back_size);
		back_coords.lt.mul(k);
		back_coords.rb.mul(k);

		draw_face(hLevelLogo_Add, back_coords, back_tex_coords, tsz);
	}
	//progress bar

	back_tex_size.set(268, 37);
	back_size.set(268, 37);
	if (b_ws)
		back_size.x *= ws_k; //ws

	back_tex_coords.lt.set(0, 768);
	back_tex_coords.rb.add(back_tex_coords.lt, back_tex_size);

	back_coords.lt.set(379, 726);
	if (b_ws)
		back_coords.lt.x *= ws_k;
	back_coords.lt.add(back_offset);

	back_coords.rb.add(back_coords.lt, back_size);
	//
	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_tex_coords.lt.x /= tsz.x;
	back_tex_coords.lt.y /= tsz.y;
	back_tex_coords.rb.x /= tsz.x;
	back_tex_coords.rb.y /= tsz.y;

	u32	Offset;
	u32	C = 0xffffffff;
	FVF::TL* pv = NULL;
	u32 v_cnt = 40;
	pv = (FVF::TL*)RCache.Vertex.Lock(2 * (v_cnt + 1), ll_hGeom2.stride(), Offset);
	FVF::TL* _pv = pv;
	float pos_delta = back_coords.width() / v_cnt;
	float tc_delta = back_tex_coords.width() / v_cnt;
	u32 clr = C;

	for (u32 idx = 0; idx<v_cnt + 1; ++idx)
	{
		clr = calc_progress_color(idx, v_cnt, owner.load_stage, owner.max_load_stage);
		pv->set(back_coords.lt.x + pos_delta*idx + offs, back_coords.rb.y + offs, 0 + EPS_S, 1, clr, back_tex_coords.lt.x + tc_delta*idx, back_tex_coords.rb.y);	pv++;
		pv->set(back_coords.lt.x + pos_delta*idx + offs, back_coords.lt.y + offs, 0 + EPS_S, 1, clr, back_tex_coords.lt.x + tc_delta*idx, back_tex_coords.lt.y);	pv++;
	}
	VERIFY(u32(pv - _pv) == 2 * (v_cnt + 1));
	RCache.Vertex.Unlock(2 * (v_cnt + 1), ll_hGeom2.stride());

	RCache.set_Shader(sh_progress);
	RCache.set_Geometry(ll_hGeom2);
	RCache.Render(D3DPT_TRIANGLESTRIP, Offset, 2 * v_cnt);


	// Draw title
	VERIFY(owner.pFontSystem);
	owner.pFontSystem->Clear();
	owner.pFontSystem->SetColor(color_rgba(157, 140, 120, 255));
	owner.pFontSystem->SetAligment(CGameFont::alCenter);
	owner.pFontSystem->OutI(0.f, 0.815f, owner.app_title);
	owner.pFontSystem->OnRender();


	//draw level-specific screenshot
	if (hLevelLogo)
	{
		Frect						r;
		r.lt.set(257, 369);

		if (b_ws)
			r.lt.x *= ws_k;
		r.lt.add(back_offset);

		r.lt.x += offs;
		r.lt.y += offs;
		back_size.set(512, 256);

		if (b_ws)
			back_size.x *= ws_k; //ws 0.625

		r.rb.add(r.lt, back_size);
		r.lt.mul(k);
		r.rb.mul(k);
		Frect						logo_tex_coords;
		logo_tex_coords.lt.set(0, 0);
		logo_tex_coords.rb.set(1, 1);

		draw_face(hLevelLogo, r, logo_tex_coords, Fvector2().set(1, 1));
	}

	//	TODO: DX10: remove this
	{
		//static int test = 0;

		//	TODO: HACK: Test failure
		//Msg("Compacting memory... Loading stage %d", test);
		//Memory.mem_compact();

		//		if (test==29)
		//		{
		//			int b=0;
		//		}

		//		VERIFY(test<3);
		//++test;
	}
}

void glApplicationRender::draw_face(ref_shader& sh, Frect& coords, Frect& tex_coords, const Fvector2& tsz)
{
	u32	Offset;
	u32	C = 0xffffffff;
	FVF::TL* pv = NULL;

	tex_coords.lt.x /= tsz.x;
	tex_coords.lt.y /= tsz.y;
	tex_coords.rb.x /= tsz.x;
	tex_coords.rb.y /= tsz.y;

	pv = (FVF::TL*) RCache.Vertex.Lock(4, ll_hGeom.stride(), Offset);
	pv->set(coords.lt.x, coords.rb.y, C, tex_coords.lt.x, tex_coords.rb.y);	pv++;
	pv->set(coords.lt.x, coords.lt.y, C, tex_coords.lt.x, tex_coords.lt.y);	pv++;
	pv->set(coords.rb.x, coords.rb.y, C, tex_coords.rb.x, tex_coords.rb.y);	pv++;
	pv->set(coords.rb.x, coords.lt.y, C, tex_coords.rb.x, tex_coords.lt.y);	pv++;
	RCache.Vertex.Unlock(4, ll_hGeom.stride());

	RCache.set_Shader(sh);
	RCache.set_Geometry(ll_hGeom);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

u32 calc_progress_color(u32 idx, u32 total, int stage, int max_stage)
{
	if (idx>(total / 2))
		idx = total - idx;


	float kk = (float(stage + 1) / float(max_stage))*(total / 2.0f);
	float f = 1 / (exp((float(idx) - kk)*0.5f) + 1.0f);

	return color_argb_f(f, 1.0f, 1.0f, 1.0f);
}
