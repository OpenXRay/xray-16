#ifndef fs2H
#define fs2H

#pragma once

class ENGINE_API IBasicStream {
private:
	std::stack<u32>	chunk_pos;
	std::stack<u16>	subchunk_pos;
	void		revbytes		( void *bp, int elsize, int elcount )
	{
		register unsigned char *p, *q;

		p = ( unsigned char * ) bp;

		if ( elsize == 2 ) {
			q = p + 1;
			while ( elcount-- ) {
				*p ^= *q;
				*q ^= *p;
				*p ^= *q;
				p += 2;
				q += 2;
			}
			return;
		}

		while ( elcount-- ) {
			q = p + elsize - 1;
			while ( p < q ) {
				*p ^= *q;
				*q ^= *p;
				*p ^= *q;
				++p;
				--q;
			}
			p += elsize >> 1;
		}
	}
public:
	virtual ~IBasicStream	()
	{
		R_ASSERT(subchunk_pos.empty());
		while (!chunk_pos.empty())
			close_chunk();
	}

	// kernel
	virtual void	write	(const void* ptr, u32 count)	= 0;
	virtual void	seek	(u32 pos)						= 0;
	virtual u32	tell	()								= 0;

	// generalized writing functions
	IC void			w_u32	(u32 d)				{	revbytes(&d,4,1); write(&d,sizeof(u32));	}
	IC void			w_u16	(u16 d)				{	revbytes(&d,2,1); write(&d,sizeof(u16));	}
	IC void			w_float	(float d)			{	revbytes(&d,4,1); write(&d,sizeof(float));	}
	IC void			w_u8	(u8 d)				{	write(&d,sizeof(u8));		}
	IC void			w_string(const char *p)
	{
    	write(p,xr_strlen(p));
		w_u8(13);
		w_u8(10);
    }
	IC void			w_stringZ(const char *p) 	{	write(p,xr_strlen(p)+1); if (!(xr_strlen(p)&1)) w_u8(0);}
	IC void			w_vector(Fvector v)			{	revbytes(&v,4,3); write(&v,3*sizeof(float));	}
	IC void			w_color	(Fcolor c)			{	revbytes(&c,4,4); write(&c,4*sizeof(float));	}

	// generalized chunking
	IC void			open_chunk	(u32 type)
	{
		w_u32(type);
		chunk_pos.push(tell());
		w_u32(0);	// the place for 'size'
	}
	IC void			close_chunk	()
	{
		VERIFY(!chunk_pos.empty());

		int pos  = tell();
		seek		(chunk_pos.top());
		w_u32		(pos-chunk_pos.top()-4);
		seek		(pos);
		chunk_pos.pop();
	}
	// generalized chunking
	IC void			open_subchunk	(u32 type)
	{
		w_u32(type);
		subchunk_pos.push((u16)tell());
		w_u16(0);	// the place for 'size'
	}
	IC void			close_subchunk	()
	{
		VERIFY(!subchunk_pos.empty());

		int pos  = tell();
		seek		(subchunk_pos.top());
		w_u16		(u16(pos-subchunk_pos.top()-2));
		seek		(pos);
		subchunk_pos.pop();
	}
	IC void			write_chunk	(u32 type, void* data, u32 size)
	{
		open_chunk	(type);
		write		(data,size);
		close_chunk	();
	}
};

class ENGINE_API CMemoryStream : public IBasicStream
{
	u8*		data;
	u32		position;
	u32		mem_size;
	u32		file_size;
public:
	CMemoryStream() {
		data		= 0;
		position	= 0;
		mem_size	= 0;
		file_size	= 0;
	}
	virtual ~CMemoryStream();

	// kernel
	virtual void	write	(const void* ptr, u32 count);

	virtual void	seek	(u32 pos)
	{	position = pos;		}
	virtual u32	tell	()
	{	return position;	}

	// specific
	u8*	pointer	()	{ return data; }
	u32	size	()	{ return file_size;	}
	void	clear	()  { file_size=0; position=0;	}
	void	SaveTo	(const char* fn)
	{
    #ifdef M_BORLAND
        int H = open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IREAD|S_IWRITE);
    #else
        int H = _open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IREAD|S_IWRITE);
    #endif
        R_ASSERT(H>0);
        _write(H,pointer(),size());
        _close(H);
	}
};

#include "lwchunks.h"

class ENGINE_API CLWMemoryStream: public CMemoryStream{
	IC void make_ord( int nbloks, int index, unsigned char *ord )
	{
		int i, d;

		for ( i = 8, d = 16; i < 128; i *= 2 ){
			if ( i >= nbloks ) break;
			d /= 2;
		}
		ord[ 0 ] = u8(128 + index * d);
		ord[ 1 ] = 0;
	}

public:
	IC void		begin_save		()				{ open_chunk(ID_FORM); w_u32(ID_LWO2);}
	IC void		end_save		(LPCSTR name)	{ close_chunk(); SaveTo(name);}
	IC void		w_layer			(u16 number, LPCSTR name){
		open_chunk(ID_LAYR);
			w_u16	(number);	// num
			w_u16	(0);
			w_float	(0);		// pivot
			w_float	(0);
			w_float	(0);
			w_stringZ(name);
		close_chunk();
	}
	IC void		w_vx   			(int idx)
	{
    	if (idx>=0xFF00){
        	idx|=0xff000000;
            w_u32(idx);
        }else
        	w_u16((u16)idx);
    }

	IC void		w_face3			(int i0, int i1, int i2)
	{	w_u16(3); w_vx(i0); w_vx(i1); w_vx(i2); }

	IC void		begin_vmap		(BOOL polymap, u32 type, int dim, LPCSTR name){
		open_chunk(polymap?ID_VMAD:ID_VMAP);
		w_u32	(type);
		w_u16	((u16)dim);
		R_ASSERT2(name&&name[0],"Empty vmap name!");
		w_stringZ(name);
	}
	IC void		end_vmap		()
	{	close_chunk();}

	IC void		w_vmap			(int v_index, int dim, float* uv)
	{	w_vx(v_index); w_float(uv[0]); if (dim==2) w_float(1.f-uv[1]); }

	IC void		w_vmad			(int v_index, int f_index, int dim, float* uv)
	{	w_vx(v_index); w_vx(f_index); w_float(uv[0]); if (dim==2) w_float(1.f-uv[1]); }

	IC void Wsurface(LPCSTR name, BOOL b2Sided, u16 image, LPCSTR vmap, LPCSTR sh_eng, LPCSTR sh_comp){
		// surf screen
		char ord[2];
		open_chunk(ID_SURF);
			w_stringZ(name);
			w_stringZ("");
			open_subchunk(ID_COLR);
 				w_float(1.f);w_float(1.f);w_float(1.f); w_u16(0);
			close_subchunk();
			open_subchunk(ID_SIDE);
				w_u16(u16(b2Sided?3:1));
			close_subchunk();
			open_subchunk(ID_BLOK);
				open_subchunk(ID_IMAP);
					make_ord(128,0,(u8*)ord);
					w_stringZ(ord);
					open_subchunk(ID_CHAN);
						w_u32(ID_COLR);
					close_subchunk();
					open_subchunk(ID_OPAC);
						w_u16(0);
						w_float(1.f);
						w_u16(0);
					close_subchunk();
					open_subchunk(ID_ENAB);
						w_u16(1);
					close_subchunk();
				close_subchunk();
				open_subchunk(ID_PROJ);
					w_u16(5);
				close_subchunk();
				open_subchunk(ID_IMAG);
					w_u16(image);
				close_subchunk();
				open_subchunk(ID_VMAP);
					w_stringZ(vmap);
				close_subchunk();
				open_subchunk(ID_AAST);
					w_u16(1); w_float(1.f);
				close_subchunk();
				open_subchunk(ID_PIXB);
					w_u16(1);
				close_subchunk();
				open_subchunk(ID_STCK);
					w_float(0.f); w_u16(0);
				close_subchunk();
				open_subchunk(ID_TAMP);
					w_float(1.f); w_u16(0);
				close_subchunk();
			close_subchunk();
			// plugins
/*			open_subchunk(ID_BLOK);
				open_subchunk(ID_SHDR);
					make_ord(128,0,(u8*)ord);
					w_stringZ(ord);
					open_subchunk(ID_ENAB);
						w_u16(1);
					close_subchunk();
					open_subchunk(ID_FUNC);
						w_stringZ("!XRayShader");
                        string64 tmp;
                        strcpy(tmp,sh_eng);
						write(tmp,sizeof(tmp));
						w_u32(-1);
                        strcpy(tmp,sh_comp);
						write(tmp,sizeof(tmp));
						w_u32(-1);
						write("",128);
					close_subchunk();
				close_subchunk();
			close_subchunk();
*/		close_chunk();
	}
};

#endif
