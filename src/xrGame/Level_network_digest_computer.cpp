#include "StdAfx.h"
#include "Level.h"
#include <GameSpy/md5.h>
#include "xrNetServer/NET_Messages.h"

extern void GetCDKey_FromRegistry(char* CDKeyStr);
char const* ComputeClientDigest(string128& dest)
{
    string128 cd_key;
    string128 md5hash;
    GetCDKey_FromRegistry(cd_key); // cd_key is not longer than 64 bytes !
    int cd_keylen = xr_strlen(cd_key);
    if (!cd_keylen)
    {
        dest[0] = 0;
        return dest;
    }
    xr_strupr(cd_key);
    MD5Digest(reinterpret_cast<unsigned char*>(cd_key), cd_keylen, md5hash);
    md5hash[33] = 0;
    xr_strcpy(dest, sizeof(dest), md5hash);
    return dest;
};

void CLevel::SendClientDigestToServer()
{
    string128 tmp_digest;
    NET_Packet P;
    P.w_begin(M_SV_DIGEST);
    m_client_digest = ComputeClientDigest(tmp_digest);
    P.w_stringZ(m_client_digest);
    SecureSend(P, net_flags(TRUE, TRUE, TRUE, TRUE));
}
