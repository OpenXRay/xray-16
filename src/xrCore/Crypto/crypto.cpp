#include "stdafx.h"
#include <cryptopp/dll.h>

namespace crypto
{
static void unsafe_xr_free(void* ptr) { xr_free(ptr); };
extern "C" XRCORE_API void GetNewAndDeleteForCryptoPP(CryptoPP::PNew& pNew, CryptoPP::PDelete& pDelete)
{
    pNew = xr_malloc;
    pDelete = unsafe_xr_free;
}

} // namespace crypto
