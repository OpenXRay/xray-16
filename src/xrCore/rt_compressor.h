#ifndef rt_compressorH
#define rt_compressorH
#pragma once

extern XRCORE_API void	rtc_initialize	();
extern XRCORE_API u32		rtc_compress	(void *dst, u32 dst_len, const void* src, u32 src_len);
extern XRCORE_API u32		rtc_decompress	(void *dst, u32 dst_len, const void* src, u32 src_len);
extern XRCORE_API u32		rtc_csize		(u32 in);

extern XRCORE_API void      rtc9_initialize	();
extern XRCORE_API void      rtc9_uninitialize	();
extern XRCORE_API u32       rtc9_compress   (void *dst, u32 dst_len, const void* src, u32 src_len);
extern XRCORE_API u32       rtc9_decompress	(void *dst, u32 dst_len, const void* src, u32 src_len);
extern XRCORE_API u32       rtc9_csize		(u32 in);

#endif
