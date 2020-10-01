#pragma once
#if !defined(AFX_NET_COMPRESSOR_H__21E1ED1C_BF92_4BF0_94A8_18A27486EBFD__INCLUDED_)
#define AFX_NET_COMPRESSOR_H__21E1ED1C_BF92_4BF0_94A8_18A27486EBFD__INCLUDED_

#include "Common/Noncopyable.hpp"
#include "xrCore/_types.h"
#include "xrCommon/xr_map.h"

class Lock;

class XRNETSERVER_API NET_Compressor : Noncopyable
{
    Lock* pcs;

    struct SCompressorStats
    {
        u32 total_uncompressed_bytes;
        u32 total_compressed_bytes;

        struct SStatPacket
        {
            u32 hit_count;
            u32 unlucky_attempts;
            u32 compressed_size;
            SStatPacket() : hit_count(0), unlucky_attempts(0), compressed_size(0) {}
        };

        xr_map<u32, SStatPacket> m_packets;

        SCompressorStats() : total_uncompressed_bytes(0), total_compressed_bytes(0) {}
        SStatPacket* get(u32 size) { return &(m_packets[size]); };
    } m_stats;

public:
    NET_Compressor();
    ~NET_Compressor();

    u16 compressed_size(const u32& count) const;
    u16 Compress(u8* dest, const u32& dest_size, u8* src, const u32& count); // return size of compressed
    u16 Decompress(u8* dest, const u32& dest_size, u8* src, const u32& count); // return size of compressed
    void DumpStats(bool brief) const;
};

#endif // !defined(AFX_NET_COMPRESSOR_H__21E1ED1C_BF92_4BF0_94A8_18A27486EBFD__INCLUDED_)
