#if !defined(__ximaCFG_h)
#define __ximaCFG_h

#include "../../../xrCore/fastdelegate.h"

extern "C" void*	cxalloc(size_t size);
extern "C" void		cxfree(void* ptr);
extern "C" void*	cxrealloc(void* ptr, size_t size);

//typedef void (*xima_progress_callback)(int progress);
//extern xima_progress_callback	xima_jpeg_encode_process_cb;

#ifdef CXIMAGE_AS_SHARED_LIBRARY //must be defined in Release_Shared configuration

#include "../../../xrCore/xrCore.h"

#pragma comment(lib,"jpeg.lib")
#pragma comment(lib,"xrcore.lib")

#ifdef DEBUG
#include "../../../xrCore/ftimer.h"
#endif //#ifdef DEBUG

#ifdef	CXIMAGE_BUILD
#	define	CXIMAGE_API	__declspec(dllexport)
#else
#	define	CXIMAGE_API	__declspec(dllimport)
#endif //#ifdef	CXIMAGE_BUILD
#else  //if CXIMAGE_AS_SHARED_LIBRARY linking as static library ...
#	define	CXIMAGE_API
#endif //CXIMAGE_AS_SHARED_LIBRARY
typedef void (*jpeg_encode_callback_func)(long);

//CXIMAGE_API jpeg_encode_callback_func g_jpeg_encode_cb;
extern "C" CXIMAGE_API fastdelegate::FastDelegate1< long > g_jpeg_encode_delegate;


/////////////////////////////////////////////////////////////////////////////
// CxImage supported features
#define CXIMAGE_SUPPORT_ALPHA          1
#define CXIMAGE_SUPPORT_SELECTION      1
#define CXIMAGE_SUPPORT_TRANSFORMATION 1
#define CXIMAGE_SUPPORT_DSP            1
#define CXIMAGE_SUPPORT_LAYERS		   1
#define CXIMAGE_SUPPORT_INTERPOLATION  1

#define CXIMAGE_SUPPORT_DECODE	1
#define CXIMAGE_SUPPORT_ENCODE	1		//<vho><T.Peck>
#define	CXIMAGE_SUPPORT_WINDOWS 1

/////////////////////////////////////////////////////////////////////////////
// CxImage supported formats
#define CXIMAGE_SUPPORT_BMP 0
#define CXIMAGE_SUPPORT_GIF 0
#define CXIMAGE_SUPPORT_JPG 1
#define CXIMAGE_SUPPORT_PNG 0
#define CXIMAGE_SUPPORT_ICO 0
#define CXIMAGE_SUPPORT_TIF 0
#define CXIMAGE_SUPPORT_TGA 0
#define CXIMAGE_SUPPORT_PCX 0
#define CXIMAGE_SUPPORT_WBMP 0
#define CXIMAGE_SUPPORT_WMF 0

#define CXIMAGE_SUPPORT_JP2 0
#define CXIMAGE_SUPPORT_JPC 0
#define CXIMAGE_SUPPORT_PGX 0
#define CXIMAGE_SUPPORT_PNM 0
#define CXIMAGE_SUPPORT_RAS 0

#define CXIMAGE_SUPPORT_JBG 0		// GPL'd see ../jbig/copying.txt & ../jbig/patents.htm

#define CXIMAGE_SUPPORT_MNG 0
#define CXIMAGE_SUPPORT_SKA 0
#define CXIMAGE_SUPPORT_RAW 0

/////////////////////////////////////////////////////////////////////////////
#define	CXIMAGE_MAX_MEMORY 268435456

#define CXIMAGE_DEFAULT_DPI 96

#define CXIMAGE_ERR_NOFILE "null file handler"
#define CXIMAGE_ERR_NOIMAGE "null image!!!"

#define CXIMAGE_SUPPORT_EXCEPTION_HANDLING 0

/////////////////////////////////////////////////////////////////////////////
//color to grey mapping <H. Muelner> <jurgene>
//#define RGB2GRAY(r,g,b) (((b)*114 + (g)*587 + (r)*299)/1000)
#define RGB2GRAY(r,g,b) (((b)*117 + (g)*601 + (r)*306) >> 10)

#endif
