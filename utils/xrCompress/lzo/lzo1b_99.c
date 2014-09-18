#include "lzoconf.h"
#if !defined(LZO_99_UNSUPPORTED)

#define COMPRESS_ID		99

#define DDBITS			3
#define CLEVEL			9

#define D_BITS			16
#define MATCH_IP_END		in_end
#include "compr1b.h"

#endif
