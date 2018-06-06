#include "stdafx.h"
#include "xrTheora_Stream.h"

#ifdef _EDITOR
//# pragma comment(lib, "x:\\oggB.lib")
//# pragma comment(lib, "x:\\theoraB.lib")
#endif

CTheoraStream::CTheoraStream()
{
    // clear self
    source = 0;
    fpms = 0.f;
    d_frame = -1;
    tm_total = 0;
    key_rate = 0;
    // start up Ogg stream synchronization layer
    ogg_sync_init(&o_sync_state);
    // init supporting Theora structures needed in header parsing
    theora_comment_init(&t_comment);
    theora_info_init(&t_info);
    // clear struct
    memset(&o_stream_state, 0, sizeof(o_stream_state));
    memset(&o_page, 0, sizeof(o_page));
    memset(&t_state, 0, sizeof(t_state));
    memset(&t_yuv_buffer, 0, sizeof(t_yuv_buffer));
}

CTheoraStream::~CTheoraStream()
{
    ogg_sync_clear(&o_sync_state);
    ogg_stream_clear(&o_stream_state);
    theora_clear(&t_state);
    theora_comment_clear(&t_comment);
    theora_info_clear(&t_info);
    FS.r_close(source);
}

void CTheoraStream::Reset()
{
    source->seek(0);
    ogg_stream_reset(&o_stream_state);
    ogg_sync_reset(&o_sync_state);
    t_state.granulepos = -1;
    d_frame = -1;
}

int CTheoraStream::ReadData()
{
    char* buffer = ogg_sync_buffer(&o_sync_state, 4096);
    long bytes = 4096 > (size_t)source->elapsed() ? source->elapsed() : 4096;
    source->r(buffer, bytes);
    ogg_sync_wrote(&o_sync_state, bytes);
    return bytes;
}

BOOL CTheoraStream::ParseHeaders()
{
    ogg_packet o_packet;
    int header_count = 0;
    BOOL stateflag = FALSE;

    // find Theora stream
    while (!stateflag)
    {
        int ret = ReadData();
        if (ret == 0)
            break;
        while (ogg_sync_pageout(&o_sync_state, &o_page) > 0)
        {
            ogg_stream_state test;

            // is this a mandated initial header? If not, stop parsing
            if (!ogg_page_bos(&o_page))
            {
                // don't leak the page; get it into the appropriate stream
                ogg_stream_pagein(&o_stream_state, &o_page);
                stateflag = 1;
                break;
            }

            ogg_stream_init(&test, ogg_page_serialno(&o_page));
            ogg_stream_pagein(&test, &o_page);
            ogg_stream_packetout(&test, &o_packet);

            // identify the codec: try theora
            if (!header_count && theora_decode_header(&t_info, &t_comment, &o_packet) >= 0)
            {
                // it is theora
                CopyMemory(&o_stream_state, &test, sizeof(test));
                header_count = 1;
            }
            else
            {
                // whatever it is, we don't care about it
                ogg_stream_clear(&test);
            }
        }
        // fall through to non-bos page parsing
    }

    // fail if theora stream not found in source
    if (0 == header_count)
        return FALSE;

    // we're expecting more header packets.
    while ((header_count && header_count < 3))
    {
        int ret;

        // look for further theora headers
        while (header_count && (header_count < 3) && 0 != (ret = ogg_stream_packetout(&o_stream_state, &o_packet)))
        {
            if (ret < 0)
            {
                fprintf(stderr, "Error parsing Theora stream headers; corrupt stream?\n");
                exit(1);
            }
            if (theora_decode_header(&t_info, &t_comment, &o_packet))
            {
                printf("Error parsing Theora stream headers; corrupt stream?\n");
                exit(1);
            }
            header_count++;
            if (header_count == 3)
                break;
        }

        // The header pages/packets will arrive before anything else we
        // care about, or the stream is not obeying spec
        if (ogg_sync_pageout(&o_sync_state, &o_page) > 0)
        {
            ogg_stream_pagein(&o_stream_state, &o_page);
        }
        else
        {
            ret = ReadData(); // someone needs more data
            if (ret == 0)
                FATAL("End of file while searching for codec headers.");
        }
    }

    if (3 != header_count)
        return FALSE;

    // init decode
    theora_decode_init(&t_state, &t_info);
    // calculate frame per ms
    fpms = ((float)t_info.fps_numerator / (float)t_info.fps_denominator) / 1000.f;

    //. XXX hack (maybe slow)
    // calculate frame count & total length in ms & key rate
    ogg_int64_t frame_count = 0;
    ogg_int64_t p_key = 0, c_key = 0;
    while (TRUE)
    {
        while (ogg_stream_packetout(&o_stream_state, &o_packet) > 0)
        {
            if ((0 == key_rate) && theora_packet_iskeyframe(&o_packet))
            {
                p_key = c_key;
                c_key = frame_count;
                key_rate = (u32)(c_key - p_key);
            }
            frame_count++;
        }
        // check eof
        if (source->eof())
            break;
        // no data yet for somebody. Grab another page
        if (0 == ReadData())
            break;
        while (ogg_sync_pageout(&o_sync_state, &o_page) > 0)
            ogg_stream_pagein(&o_stream_state, &o_page);
    }
    tm_total = iFloor(frame_count / fpms);

    // seek to 0
    Reset();

    return TRUE;
}

BOOL CTheoraStream::Decode(u32 in_tm_play)
{
    VERIFY(in_tm_play < tm_total);
    ogg_int64_t t_frame;
    t_frame = iFloor(in_tm_play * fpms);
    ogg_int64_t k_frame = t_frame - t_frame % key_rate;

    if (d_frame < t_frame)
    {
        BOOL result = FALSE;
        ogg_packet o_packet;
        while (d_frame < t_frame)
        {
            while (FALSE == result)
            {
                // theora is one in, one out...
                if (ogg_stream_packetout(&o_stream_state, &o_packet) > 0 && !theora_packet_isheader(&o_packet))
                {
                    d_frame++;
                    //. hack preroll
                    if (d_frame < k_frame)
                    {
                        //. dbg_log ((stderr,"%04d: preroll\n",d_frame));
                        VERIFY((0 != d_frame % key_rate) ||
                            (0 == d_frame % key_rate) && theora_packet_iskeyframe(&o_packet));
                        continue;
                    }
                    BOOL is_key = theora_packet_iskeyframe(&o_packet);
                    VERIFY((d_frame != k_frame) || ((d_frame == k_frame) && is_key));
                    // real decode
                    //. dbg_log ((stderr,"%04d: decode\n",d_frame));
                    int res = theora_decode_packetin(&t_state, &o_packet);
                    VERIFY(res != OC_BADPACKET);
                    //. dbg_log ((stderr,"%04d: granule frame\n",theora_granule_frame(&t_state,t_state.granulepos)));
                    if (d_frame >= t_frame)
                        result = TRUE;
                }
                else
                    break;
            }
            // check eof
            VERIFY(!(FALSE == result && source->eof()));
            if (FALSE == result)
            {
                // no data yet for somebody. Grab another page
                if (ReadData())
                {
                    while (ogg_sync_pageout(&o_sync_state, &o_page) > 0)
                        ogg_stream_pagein(&o_stream_state, &o_page);
                }
            }
        }
        // all right - get yuv buffer
        VERIFY(TRUE == result);
        VERIFY(d_frame == t_frame);
        theora_decode_YUVout(&t_state, &t_yuv_buffer);
        //. dbg_log ((stderr,"%04d: yuv out\n",d_frame));
        return TRUE;
    }
    return FALSE;
}

BOOL CTheoraStream::Load(const char* fname)
{
    VERIFY(0 == source);
// open source
#ifdef _EDITOR
    source = FS.r_open(0, fname);
#else
    source = FS.rs_open(0, fname);
#endif
    VERIFY(source);

    // parse headers
    BOOL res = ParseHeaders();
    // seek to start
    Reset();
    return res;
}
