#include "stdafx.h"
#include "Level.h"
#include "xrCore/Compression/ppmd_compressor.h"
#include "xrPhysics/iphworld.h"
#include "xrServer_updates_compressor.h"

void CLevel::ProcessCompressedUpdate(NET_Packet& P, u8 const compress_type)
{
    NET_Packet uncompressed_packet;
    u16 next_size;
    P.r_u16(next_size);
    stats.ClientCompressor.Begin();
    while (next_size)
    {
        if (compress_type & eto_ppmd_compression)
        {
            R_ASSERT(m_trained_stream);
            uncompressed_packet.B.count = ppmd_trained_decompress(uncompressed_packet.B.data,
                sizeof(uncompressed_packet.B.data), P.B.data + P.r_tell(), next_size, m_trained_stream);
        }
        else if (compress_type & eto_lzo_compression)
        {
            R_ASSERT(m_lzo_dictionary.data);
            uncompressed_packet.B.count = sizeof(uncompressed_packet.B.data);
            lzo_decompress_dict(P.B.data + P.r_tell(), next_size, uncompressed_packet.B.data,
                uncompressed_packet.B.count, m_lzo_working_memory, m_lzo_dictionary.data, m_lzo_dictionary.size);
        }
        else
        {
            NODEFAULT;
        }

        VERIFY2(
            uncompressed_packet.B.count <= sizeof(uncompressed_packet.B.data), "stack owerflow after decompressing");

        P.r_seek(P.r_tell() + next_size);
        uncompressed_packet.r_seek(0);
        Objects.net_Import(&uncompressed_packet);
        P.r_u16(next_size);
    }
    stats.ClientCompressor.End();

    if (OnClient())
        UpdateDeltaUpd(timeServer());
    // XXX: Is a COPY of IClientStatistic really intended here?!
    IClientStatistic pStat = Level().GetStatistic();
    u32 dTime = 0;

    if ((Level().timeServer() + pStat.getPing()) < P.timeReceive)
    {
        dTime = pStat.getPing();
    }
    else
    {
        dTime = Level().timeServer() - P.timeReceive + pStat.getPing();
    }
    u32 NumSteps = physics_world()->CalcNumSteps(dTime);
    SetNumCrSteps(NumSteps);
}

void CLevel::init_compression()
{
    compression::init_ppmd_trained_stream(m_trained_stream);
    compression::init_lzo(m_lzo_working_memory, m_lzo_working_buffer, m_lzo_dictionary);
}

void CLevel::deinit_compression()
{
    if (m_trained_stream)
    {
        compression::deinit_ppmd_trained_stream(m_trained_stream);
    }
    if (m_lzo_working_buffer)
    {
        VERIFY(m_lzo_dictionary.data);
        compression::deinit_lzo(m_lzo_working_buffer, m_lzo_dictionary);
    }
}
