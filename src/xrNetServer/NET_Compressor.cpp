// NET_Compressor.cpp: implementation of the NET_Compressor class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "NET_Common.h"
#include "NET_Compressor.h"
#include "xrCore/Threading/Lock.hpp"

#if NET_USE_COMPRESSION

#ifdef DEBUG
#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)
#endif // DEBUG

#if NET_USE_LZO_COMPRESSION
#define ENCODE rtc9_compress
#define DECODE rtc9_decompress
#else // NET_USE_LZO_COMPRESSION
#include "xrCore/ppmd_compressor.h"
#define ENCODE ppmd_compress
#define DECODE ppmd_decompress
#endif // NET_USE_LZO_COMPRESSION

#endif // NET_USE_COMPRESSION

#if 1 // def DEBUG
// static FILE*    OriginalTrafficDump     = NULL;
// static FILE*    CompressedTrafficDump   = NULL;
static FILE* RawTrafficDump = nullptr;
static FILE* CompressionDump = nullptr;
#endif // DEBUG

#define NOWARN

// size of range encoding code values

#define PPM_CODE_BITS 32
#define PPM_TOP_VALUE ((NET_Compressor::code_value)1 << (PPM_CODE_BITS - 1))

#define SHIFT_BITS (PPM_CODE_BITS - 9)
#define EXTRA_BITS ((PPM_CODE_BITS - 2) % 8 + 1)
#define PPM_BOTTOM_VALUE (PPM_TOP_VALUE >> 8)

/*
// c is written as first byte in the datastream
// one could do without c, but then you have an additional if
// per outputbyte.
void NET_Compressor::start_encoding(BYTE* dest, u32 header_size)
{
    dest += header_size - 1;
    RNGC.low = 0; // Full code range
    RNGC.range = PPM_TOP_VALUE;
    RNGC.buffer = 0;
    RNGC.help = 0; // No bytes to follow
    RNGC.bytecount = 0;
    RNGC.ptr = dest;
}

// I do the normalization before I need a defined state instead of
// after messing it up. This simplifies starting and ending.
void NET_Compressor::encode_normalize()
{
    while (RNGC.range <= PPM_BOTTOM_VALUE) // do we need renormalisation?
    {
        if (RNGC.low < code_value(0xff) << SHIFT_BITS) // no carry possible --> output
        {
            RNGC.byte_out(RNGC.buffer);
            for (; RNGC.help; RNGC.help--)
                RNGC.byte_out(0xff);
            RNGC.buffer = (BYTE)(RNGC.low >> SHIFT_BITS);
        }
        else if (RNGC.low & PPM_TOP_VALUE) // carry now, no future carry
        {
            RNGC.byte_out(RNGC.buffer + 1);
            for (; RNGC.help; RNGC.help--)
                RNGC.byte_out(0);
            RNGC.buffer = (BYTE)(RNGC.low >> SHIFT_BITS);
        }
        else // passes on a potential carry
        {
            RNGC.help++;
        }

        RNGC.range <<= 8;
        RNGC.low = (RNGC.low << 8) & (PPM_TOP_VALUE - 1);
        RNGC.bytecount ++;
    }
}

// Encode a symbol using frequencies
// sy_f is the interval length (frequency of the symbol)
// lt_f is the lower end (frequency sum of < symbols)
// tot_f is the total interval length (total frequency sum)
// or (faster): tot_f = (code_value)1<<shift
void NET_Compressor::encode_freq(freq sy_f, freq lt_f, freq tot_f)
{
    encode_normalize();

    code_value r = RNGC.range / tot_f;
    code_value tmp = r * lt_f;

    RNGC.low += tmp;

    if (lt_f + sy_f < tot_f) RNGC.range = r * sy_f;
    else RNGC.range -= tmp;
}

void NET_Compressor::encode_shift(freq sy_f, freq lt_f, freq shift)
{
    encode_normalize();

    code_value r = RNGC.range >> shift;
    code_value tmp = r * lt_f;

    RNGC.low += tmp;

    if ((lt_f + sy_f) >> shift) RNGC.range -= tmp;
    else RNGC.range = r * sy_f;
}

// Finish encoding
// actually not that many bytes need to be output, but who
// cares. I output them because decode will read them :)
// the return value is the number of bytes written
u32 NET_Compressor::done_encoding()
{
    encode_normalize(); // now we have a normalized state

    RNGC.bytecount += 3;

    u32 tmp = ((RNGC.low & (PPM_BOTTOM_VALUE - 1)) < ((RNGC.bytecount & 0xffffffL) >> 1))
                  ? (RNGC.low >> SHIFT_BITS)
                  : (RNGC.low >> SHIFT_BITS) + 1;

    if (tmp > 0xff) // we have a carry
    {
        RNGC.byte_out(RNGC.buffer + 1);

        for (; RNGC.help; RNGC.help--)
            RNGC.byte_out(0);
    }
    else // no carry
    {
        RNGC.byte_out(RNGC.buffer);

        for (; RNGC.help; RNGC.help--)
            RNGC.byte_out(0xff);
    }

    RNGC.byte_out((BYTE)(tmp & 0xff));
    RNGC.byte_out(0);

    return RNGC.bytecount;
}

// Start the decoder
int NET_Compressor::start_decoding(BYTE* src, u32 header_size)
{
    src += header_size;
    RNGC.ptr = src;
    RNGC.buffer = RNGC.byte_in();
    RNGC.low = RNGC.buffer >> (8 - EXTRA_BITS);
    RNGC.range = (code_value)1 << EXTRA_BITS;
    return 0;
}

void NET_Compressor::decode_normalize()
{
    while (RNGC.range <= PPM_BOTTOM_VALUE)
    {
        RNGC.low = (RNGC.low << 8) | ((RNGC.buffer << EXTRA_BITS) & 0xff);
        RNGC.buffer = RNGC.byte_in();
        RNGC.low |= RNGC.buffer >> (8 - EXTRA_BITS);
        RNGC.range <<= 8;
    }
}


// Calculate culmulative frequency for next symbol. Does NO update!
// tot_f is the total frequency
// or: totf is (code_value)1<<shift
// returns the culmulative frequency
NET_Compressor::freq NET_Compressor::decode_culfreq(freq tot_f)
{
    decode_normalize();
    RNGC.help = RNGC.range / tot_f;

    freq tmp = RNGC.low / RNGC.help;

    return (tmp >= tot_f) ? (tot_f - 1) : (tmp);
}

NET_Compressor::freq NET_Compressor::decode_culshift(freq shift)
{
    decode_normalize();
    RNGC.help = RNGC.range >> shift;

    freq tmp = RNGC.low / RNGC.help;

    return (tmp >> shift) ? ((code_value(1) << shift) - 1) : (tmp);
}


// Update decoding state
// sy_f is the interval length (frequency of the symbol)
// lt_f is the lower end (frequency sum of < symbols)
// tot_f is the total interval length (total frequency sum)
void NET_Compressor::decode_update(freq sy_f, freq lt_f, freq tot_f)
{
    code_value tmp = RNGC.help * lt_f;

    RNGC.low -= tmp;

    if (lt_f + sy_f < tot_f) RNGC.range = RNGC.help * sy_f;
    else RNGC.range -= tmp;
}


// Decode a byte/short without modelling
BYTE NET_Compressor::decode_byte()
{
    u32 tmp = decode_culshift(8);

    decode_update(1, tmp, (freq)1 << 8);

    return BYTE(tmp);
}

u16 NET_Compressor::decode_short()
{
    u32 tmp = decode_culshift(16);

    decode_update(1, tmp, (freq)1 << 16);
    return u16(tmp);
}


// Finish decoding
// rc is the range coder to be used
void NET_Compressor::done_decoding()
{
    decode_normalize(); // normalize to use up all bytes
}
*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef CONFIG_PROFILE_LOCKS
NET_Compressor::NET_Compressor() : pcs(new Lock(MUTEX_PROFILE_ID(NET_Compressor))) {}
#else
NET_Compressor::NET_Compressor() : pcs(new Lock) {}
#endif

NET_Compressor::~NET_Compressor()
{
#if 1 // def DEBUG
    //if (strstr(Core.Params, "-dump_traffic"))
    //{
    //    fclose(OriginalTrafficDump);
    //    fclose(CompressedTrafficDump);
    //}
    if (CompressionDump)
    {
        fclose(CompressionDump);
        CompressionDump = nullptr;
    }
    if (RawTrafficDump)
    {
        fclose(RawTrafficDump);
        RawTrafficDump = nullptr;
    }
#endif // DEBUG
	delete pcs;
}

/*
void NET_Compressor::Initialize	()
{
    pcs->Enter();

#if 1//def DEBUG
    if (strstr(Core.Params, "-dump_traffic"))
    {
        OriginalTrafficDump = fopen("x:/network_out_original.dat", "wb");
        CompressedTrafficDump = fopen("x:/network_out_compressed.dat", "wb");
    }
#endif // DEBUG

    pcs->Leave();
}*/

u16 NET_Compressor::compressed_size(const u32& count) const
{
#if NET_USE_COMPRESSION

#if NET_USE_LZO_COMPRESSION
    u32 result = rtc_csize(count) + 1;
#else // NET_USE_LZO_COMPRESSION
    u32 result = 64 + (count / 8 + 1) * 10;
#endif // NET_USE_LZO_COMPRESSION

    R_ASSERT(result <= u32(u16(-1)));

    return (u16)result;

#else
    return (u16)count;
#endif // #if NET_USE_COMPRESSION
}

XRNETSERVER_API BOOL g_net_compressor_enabled = FALSE;
XRNETSERVER_API BOOL g_net_compressor_gather_stats = FALSE;

u16 NET_Compressor::Compress(BYTE* dest, const u32& dest_size, BYTE* src, const u32& count)
{
    SCompressorStats::SStatPacket* _p = nullptr;
    bool b_compress_packet = (count > 36);
    if (g_net_compressor_gather_stats && b_compress_packet)
    {
        _p = m_stats.get(count);
        _p->hit_count += 1;
        m_stats.total_uncompressed_bytes += count;
    }

    VERIFY(dest);
    VERIFY(src);
    VERIFY(count);

#if 0 // def DEBUG
    if (strstr(Core.Params, "-dump_traffic"))
    {
        fwrite(src, count, 1, OriginalTrafficDump);
        fflush(OriginalTrafficDump);
    }
#endif // DEBUG

#if !NET_USE_COMPRESSION

    CopyMemory(dest, src, count);
    return (u16(count));

#else // !NET_USE_COMPRESSION

    R_ASSERT(dest_size >= compressed_size(count));

    u32 compressed_size = count;
    u32 offset = 1;

#if NET_USE_COMPRESSION_CRC
    offset += sizeof(u32);
#endif // NET_USE_COMPRESSION_CRC

    if (!psNET_direct_connect && g_net_compressor_enabled && b_compress_packet)
    {
        pcs->Enter();
        compressed_size = offset + ENCODE(dest + offset, dest_size - offset, src, count);

        if (g_net_compressor_gather_stats)
            m_stats.total_compressed_bytes += compressed_size;

        pcs->Leave();
    }

    if (compressed_size < count)
    {
        *dest = NET_TAG_COMPRESSED;

#if NET_USE_COMPRESSION_CRC
        u32 crc = crc32(dest + offset, compressed_size);
        *((u32*)(dest + 1)) = crc;
#endif // NET_USE_COMPRESSION_CRC

#if NET_LOG_COMPRESSION
        Msg("#compress %u->%u  %02X (%08X)", count, compressed_size, *dest, *((u32*)(src + 1)));
#endif
#if NET_DUMP_COMPRESSION
#if NET_USE_LZO_COMPRESSION
        static const char* compressor_name = "LZO";
#else
        static const char* compressor_name = "PPMd";
#endif

        if (!CompressionDump)
            CompressionDump = fopen("net-compression.log", "w+b");

        fprintf(CompressionDump, "%s compress %2.0f%% %u->%u\r\n", compressor_name,
            100.0f * float(compressed_size) / float(count), count, compressed_size);
#endif // NET_DUMP_COMPRESSION
    }
    else
    {
        if (g_net_compressor_gather_stats && b_compress_packet)
            _p->unlucky_attempts += 1;

        *dest = NET_TAG_NONCOMPRESSED;

        compressed_size = count + 1;
        CopyMemory(dest + 1, src, count);

#if NET_LOG_COMPRESSION
        Msg("#compress/as-is %u->%u  %02X", count, compressed_size, *dest);
#endif
    }
    if (g_net_compressor_gather_stats && b_compress_packet)
        _p->compressed_size += compressed_size;

#if 0 // def DEBUG
    if (strstr(Core.Params, "-dump_traffic"))
    {
        fwrite(dest, compressed_size, 1, CompressedTrafficDump);
        fflush(CompressedTrafficDump);
    }
#endif // DEBUG

#if 0 //def DEBUG
    BYTE* src_back = (BYTE*)_alloca(count);
    Decompress(src_back, count, dest, compressed_size);
    BYTE* I = src_back;
    BYTE* E = src_back + count;
    BYTE* J = src;
    for (; I != E; ++I , ++J)
    VERIFY (*I == *J);

    pcs->Leave();
#endif // DEBUG

    return (u16(compressed_size));

#endif // if !NET_USE_COMPRESSION
}

u16 NET_Compressor::Decompress(BYTE* dest, const u32& dest_size, BYTE* src, const u32& count)
{
    VERIFY(dest);
    VERIFY(src);
    VERIFY(count);

#if NET_LOG_COMPRESSION
    Msg("#decompress %u  %02X (%08X)", count, src[0], *((u32*)(src + 1)));
#endif

#if NET_USE_COMPRESSSION
    if (src[0] != NET_TAG_COMPRESSED && src[0] != NET_TAG_NONCOMPRESSED)
    {
        Msg("! invalid compression-tag %02X", src[0]);
        DEBUG_BREAK;
    }
#endif // NET_USE_COMPRESSSION

#if !NET_USE_COMPRESSION

    CopyMemory(dest, src, count);

    return (u16(count));

#else

    if (*src != NET_TAG_COMPRESSED)
    {
        if (count)
        {
            CopyMemory(dest, src + 1, count - 1);
            return (u16(count - 1));
        }

        return (0);
    }

    u32 offset = 1;

#if NET_USE_COMPRESSION_CRC
    offset += sizeof(u32);
#endif // NET_USE_COMPRESSION_CRC

#if NET_USE_COMPRESSION_CRC
    u32 crc = crc32(src + offset, count);
    //Msg("decompressed %d -> ? [0x%08x]", count, crc);
    if (crc != *((u32*)(src + 1)))
        Msg("!CRC mismatch");

    R_ASSERT2(crc == *((u32*)(src + 1)), make_string("crc is different! (0x%08x != 0x%08x)", crc, *((u32*)(src + 1))));
#endif // NET_USE_COMPRESSION_CRC

    pcs->Enter();
    u32 uncompressed_size = DECODE(dest, dest_size, src + offset, count - offset);
    pcs->Leave();

    return (u16)uncompressed_size;

#endif // !NET_USE_COMPRESSION
}

void NET_Compressor::DumpStats(bool brief) const
{
    auto cit = m_stats.m_packets.cbegin();
    auto cit_e = m_stats.m_packets.cend();

    Msg("---------NET_Compressor::DumpStats-----------");

    Msg("Active=[%s]", g_net_compressor_enabled ? "yes" : "no");

    Msg("uncompressed [%d]", m_stats.total_uncompressed_bytes);
    Msg("compressed   [%d]", m_stats.total_compressed_bytes);

    u32 total_hits = 0;
    u32 unlucky_hits = 0;

    for (; cit != cit_e; ++cit)
    {
        total_hits += cit->second.hit_count;
        unlucky_hits += cit->second.unlucky_attempts;
        if (!brief)
        {
            Msg("size[%d] count[%d] unlucky[%d] avg_c[%d]", cit->first, cit->second.hit_count,
                cit->second.unlucky_attempts, iFloor(float(cit->second.compressed_size) / float(cit->second.hit_count)));
        }
    }
    Msg("total   [%d]", total_hits);
    Msg("unlucky [%d]", unlucky_hits);
}
