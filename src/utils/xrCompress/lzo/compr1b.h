
#define LZO_NEED_DICT_H
#include "config1b.h"


#if !defined(COMPRESS_ID)
#define COMPRESS_ID		_LZO_ECONCAT2(DD_BITS,CLEVEL)
#endif


#include "lzo1b_c.ch"


/***********************************************************************
//
************************************************************************/

#define LZO_COMPRESS \
	_LZO_ECONCAT3(lzo1b_,COMPRESS_ID,_compress)

#define LZO_COMPRESS_FUNC \
	_LZO_ECONCAT3(_lzo1b_,COMPRESS_ID,_compress_func)



/***********************************************************************
//
************************************************************************/

const lzo_compress_t LZO_COMPRESS_FUNC = do_compress;

LZO_PUBLIC(int)
LZO_COMPRESS ( const lzo_byte *in,  lzo_uint  in_len,
                     lzo_byte *out, lzo_uintp out_len,
                     lzo_voidp wrkmem )
{
#if defined(__LZO_QUERY_COMPRESS)
	if (__LZO_IS_COMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
		return __LZO_QUERY_COMPRESS(in,in_len,out,out_len,wrkmem,D_SIZE,lzo_sizeof(lzo_dict_t));
#endif

	return _lzo1b_do_compress(in,in_len,out,out_len,wrkmem,do_compress);
}

/*
vi:ts=4:et
*/
