// file: targasaver.cpp
#include "stdafx.h"
#pragma hdrstop

#include "tga.h"
/*
void	tga_save	(LPCSTR name, u32 w, u32 h, void* data, BOOL alpha )
{
	// Save
	TGAdesc		tga;
	tga.data	= data;
	tga.format	= alpha?IMG_32B:IMG_24B;
	tga.height	= h;
	tga.width	= w;
	tga.scanlenght=w*4;

	int		hf	= _open(name,O_CREAT | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
	tga.maketga	(hf);
	_close		(hf);
}
*/

void TGAdesc::maketga( IWriter& fs ){
	R_ASSERT(data);
	R_ASSERT(width);
	R_ASSERT(height);

	tgaHeader hdr;
	ZeroMemory( &hdr, sizeof(hdr) );
	hdr.tgaImgType			= 2;
	hdr.tgaImgSpec.tgaXSize = u16(width);
	hdr.tgaImgSpec.tgaYSize = u16(height);

	if( format == IMG_24B ){
		hdr.tgaImgSpec.tgaDepth = 24;
		hdr.tgaImgSpec.tgaImgDesc = 32;			// flip
	}
	else{
		hdr.tgaImgSpec.tgaDepth = 32;
		hdr.tgaImgSpec.tgaImgDesc = 0x0f | 32;	// flip
	}

	fs.w(&hdr, sizeof(hdr) );

	if( format==IMG_24B ){
		BYTE ab_buffer[4]={0,0,0,0};
		int  real_sl = ((width*3)) & 3;
		int  ab_size = real_sl ? 4-real_sl : 0 ;
		for( int j=0; j<height; j++){
			BYTE *p = (LPBYTE)data + scanlenght*j;
			for( int i=0; i<width; i++){
				BYTE buffer[3] = {p[0],p[1],p[2]};
				fs.w(buffer, 3 );
				p+=4;
			}
			if(ab_size)
				fs.w(ab_buffer, ab_size );
		}
	}
	else{
		if (width*4 == scanlenght)	fs.w	(data,width*height*4);
		else {
			// bad pitch, it seems :(
			for( int j=0; j<height; j++){
				BYTE *p = (LPBYTE)data + scanlenght*j;
				for( int i=0; i<width; i++){
					BYTE buffer[4] = {p[0],p[1],p[2],p[3]};
					fs.w(buffer, 4 );
					p+=4;
				}
			}
		}
	}
}

/*
void TGAdesc::maketga( int hf ){
	R_ASSERT(data);
	R_ASSERT(width);
	R_ASSERT(height);

	tgaHeader hdr;
	ZeroMemory( &hdr, sizeof(hdr) );
	hdr.tgaImgType			= 2;
	hdr.tgaImgSpec.tgaXSize = u16(width);
	hdr.tgaImgSpec.tgaYSize = u16(height);

	if( format == IMG_24B ){
		hdr.tgaImgSpec.tgaDepth = 24;
		hdr.tgaImgSpec.tgaImgDesc = 32;			// flip
	}
	else{
		hdr.tgaImgSpec.tgaDepth = 32;
		hdr.tgaImgSpec.tgaImgDesc = 0x0f | 32;	// flip
	}

	_write(hf, &hdr, sizeof(hdr) );

	if( format==IMG_24B ){
		BYTE ab_buffer[4]={0,0,0,0};
		int  real_sl = ((width*3)) & 3;
		int  ab_size = real_sl ? 4-real_sl : 0 ;
		for( int j=0; j<height; j++){
			BYTE *p = (LPBYTE)data + scanlenght*j;
			for( int i=0; i<width; i++){
				BYTE buffer[3] = {p[0],p[1],p[2]};
				_write(hf, buffer, 3 );
				p+=4;
			}
			if(ab_size)
				_write(hf, ab_buffer, ab_size );
		}
	}
	else{
		_write	(hf,data,width*height*4);
	}
}
*/