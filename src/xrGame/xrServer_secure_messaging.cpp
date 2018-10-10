#include "StdAfx.h"
#include "xrServer.h"
#include "xrMessages.h"

void xrServer::PerformSecretKeysSync(xrClientData* xrCL)
{
    VERIFY(xrCL);
    xrCL->m_last_key_sync_request_seed = m_seed_generator.genrate();
    // secure_messaging::generate_key	(new_seed, xrCL->m_secret_key);

    NET_Packet key_sync_command;
    key_sync_command.w_begin(M_SECURE_KEY_SYNC);
    key_sync_command.w_s32(xrCL->m_last_key_sync_request_seed);
    SendTo(xrCL->ID, key_sync_command);
}

void xrServer::PerformSecretKeysSyncAck(xrClientData* xrCL, NET_Packet& P)
{
    VERIFY(xrCL);
    s32 new_seed;
    P.r_s32(new_seed); // only for DEBUG
    VERIFY2(new_seed == xrCL->m_last_key_sync_request_seed, "cracker detected !");
    secure_messaging::generate_key(xrCL->m_last_key_sync_request_seed, xrCL->m_secret_key);
}

void xrServer::SecureSendTo(xrClientData* xrCL, NET_Packet& P, u32 dwFlags, u32 dwTimeout)
{
    VERIFY(xrCL);

    NET_Packet enc_packet;

    enc_packet.w_begin(M_SECURE_MESSAGE);
    enc_packet.w(P.B.data, P.B.count);
    u32 checksum = secure_messaging::encrypt(
        enc_packet.B.data + sizeof(u16), enc_packet.B.count - sizeof(u16), xrCL->m_secret_key);
    enc_packet.w_u32(checksum);
    SendTo(xrCL->ID, enc_packet, dwFlags, dwTimeout);
}

void xrServer::OnSecureMessage(NET_Packet& P, xrClientData* xrClSender)
{
#ifdef DEBUG
    char dbg_tmp_buff[33];
    ZeroMemory(dbg_tmp_buff, sizeof(dbg_tmp_buff));
    xr_strcpy(dbg_tmp_buff, "xray crypt check");
    u32 dbg_encrypt_checksum = secure_messaging::encrypt(dbg_tmp_buff, sizeof(dbg_tmp_buff), xrClSender->m_secret_key);
    u32 dbg_decrypt_checksum = secure_messaging::decrypt(dbg_tmp_buff, sizeof(dbg_tmp_buff), xrClSender->m_secret_key);
    VERIFY(dbg_encrypt_checksum == dbg_decrypt_checksum);
#endif
    NET_Packet dec_packet;
    dec_packet.B.count = P.B.count - sizeof(u16) - sizeof(u32); // - r_begin - crypt_check_sum
    P.r(dec_packet.B.data, dec_packet.B.count);
    u32 checksum = secure_messaging::decrypt(dec_packet.B.data, dec_packet.B.count, xrClSender->m_secret_key);
    u32 real_checksum = 0;
    P.r_u32(real_checksum);
    VERIFY2(checksum == real_checksum, "caught cheater");
    if (checksum != real_checksum)
        return; // WARNING!: do not add any log messages - security treat!

    OnMessage(dec_packet, xrClSender->ID);
}
