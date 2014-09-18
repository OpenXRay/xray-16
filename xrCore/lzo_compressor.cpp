#include "stdafx.h"
#include "lzo_compressor.h"

int lzo_compress_dict(const lzo_bytep in , lzo_uint  in_len,
					  lzo_bytep out, lzo_uintp out_len,
					  lzo_voidp wrkmem,
					  const lzo_bytep dict, lzo_uint dict_len)
{
	return lzo1x_999_compress_dict(
		in, in_len, out, out_len, wrkmem, dict, dict_len
	);
}

int lzo_decompress_dict(const lzo_bytep in,  lzo_uint  in_len,
						lzo_bytep out, lzo_uintp out_len,
						lzo_voidp wrkmem /* NOT USED */,
						const lzo_bytep dict, lzo_uint dict_len)
{
	return lzo1x_decompress_dict_safe(
		in, in_len, out, out_len, wrkmem, dict, dict_len
	);
}

int lzo_initialize()
{
	return lzo_init();
}

