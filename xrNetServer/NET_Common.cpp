
    #include "stdafx.h"
        
    #include "NET_Common.h"

/*#ifdef DEBUG
void PrintParsedPacket(const char* message, u16 message_type, const void* packet_data, u32 packet_size)
{
	NET_Packet			tNetPacket;
	tNetPacket.construct		( packet_data, packet_size );	
	u16 msg_type;
	tNetPacket.r_begin			(msg_type);
	if (msg_type == message_type)
	{
		if (message_type == 1) //M_SPAWN
		{
			shared_str			s_name;
			tNetPacket.r_stringZ		(s_name			);
			
			string256					temp;
			tNetPacket.r_stringZ		(temp);
			u8							temp_gt;
			tNetPacket.r_u8				(temp_gt		);
			u8							s_RP;
			tNetPacket.r_u8				(s_RP			);
			Fvector						o_Position, o_Angle;
			tNetPacket.r_vec3			(o_Position		);
			tNetPacket.r_vec3			(o_Angle		);
			u16							RespawnTime, ID;
			tNetPacket.r_u16			(RespawnTime	);
			tNetPacket.r_u16			(ID				);
			
			Msg("%s M_SPAWN for [%s]-[%d]", message, s_name.c_str(), ID);
		} else
		{
			Msg("%s NOT_IMPLEMENTED_PRINT type[%d]", message, message_type);
		}
	}
}
#endif*/
//==============================================================================

#pragma pack( push )
#pragma pack( 1 )
struct
MultipacketHeader
{
    u8  tag;
    u16 unpacked_size;
};
#pragma pack( pop )


//==============================================================================

static NET_Compressor   Compressor;
static const unsigned   MaxMultipacketSize          = 32768;

XRNETSERVER_API int     psNET_GuaranteedPacketMode  = NET_GUARANTEEDPACKET_DEFAULT;


//------------------------------------------------------------------------------

MultipacketSender::MultipacketSender()
{
}


//------------------------------------------------------------------------------

void
MultipacketSender::SendPacket( const void* packet_data, u32 packet_sz, u32 flags, u32 timeout )
{
    _buf_cs.Enter();

	//PrintParsedPacket("-- LL Sending:", 1, packet_data, packet_sz);

    Buffer* buf = &_buf;

    switch( psNET_GuaranteedPacketMode )
    {
        case NET_GUARANTEEDPACKET_IGNORE :
        {
            flags &= ~DPNSEND_GUARANTEED;
        }   break;

        case NET_GUARANTEEDPACKET_SEPARATE :
        {
            if( flags & DPNSEND_GUARANTEED )
                buf = &_gbuf;
        }   break;
    }
        
    u32 old_flags = (buf->last_flags) & (~DPNSEND_IMMEDIATELLY);
    u32 new_flags = flags & (~DPNSEND_IMMEDIATELLY);

    if(     (buf->buffer.B.count + packet_sz + sizeof(u16) >= NET_PacketSizeLimit)
        ||  (old_flags != new_flags)
        ||  (flags & DPNSEND_IMMEDIATELLY)
      )
    {
        _FlushSendBuffer( timeout, buf );
    }

    buf->buffer.w_u16( (u16)packet_sz );
    buf->buffer.w( packet_data, packet_sz );
        
    if( flags & DPNSEND_IMMEDIATELLY )
        _FlushSendBuffer( timeout, buf );
    
    buf->last_flags = flags;
    _buf_cs.Leave();
}


//------------------------------------------------------------------------------

void            
MultipacketSender::FlushSendBuffer( u32 timeout )
{
    _buf_cs.Enter();
    
    _FlushSendBuffer( timeout, &_buf );
    _FlushSendBuffer( timeout, &_gbuf );
    
    _buf_cs.Leave();
}


//------------------------------------------------------------------------------

void            
MultipacketSender::_FlushSendBuffer( u32 timeout, Buffer* buf )
{
    // expected to be called between '_buf_cs' enter/leave

    if( buf->buffer.B.count )
    {
        // compress data

        unsigned            comp_sz     = Compressor.compressed_size( buf->buffer.B.count );        
        u8                  packet_data[MaxMultipacketSize];
        MultipacketHeader*  header      = (MultipacketHeader*)packet_data;

        R_ASSERT(comp_sz < sizeof(packet_data)-sizeof(MultipacketHeader));
        R_ASSERT(comp_sz < 65535);

        comp_sz = Compressor.Compress( packet_data+sizeof(MultipacketHeader), sizeof(packet_data)-sizeof(MultipacketHeader), 
                                       buf->buffer.B.data, buf->buffer.B.count 
                                     );

        header->tag             = NET_TAG_MERGED;
        header->unpacked_size   = (u16)buf->buffer.B.count;


        // dump/log if needed

        #if NET_LOG_PACKETS
        Msg( "#send %smulti-packet %u    flags= %08X", 
             (buf->last_flags & DPNSEND_IMMEDIATELLY)?"IMMEDIATE ":"",
             buf->buffer.B.count, buf->last_flags 
           );
        #endif // NET_LOG_PACKETS

	    if( strstr( Core.Params,"-dump_traffic") ) 
        {
            static bool first_time  = true;
			FILE*       dump        = fopen( "raw-out-traffic.bins", (first_time)?"wb":"ab" );

            if( first_time )
            {
                fwrite( "BINS", 4, 1, dump );
                first_time = false;
            }

            u16 sz = u16(buf->buffer.B.count);
            
            fwrite( &sz, sizeof(u16), 1, dump );
            fwrite( buf->buffer.B.data, buf->buffer.B.count, 1, dump );
			fclose( dump );
        }


        // do send
        
        _SendTo_LL( packet_data, comp_sz+sizeof(MultipacketHeader), buf->last_flags, timeout );
        buf->buffer.B.count = 0;        
    } // if buffer not empty
}


//------------------------------------------------------------------------------

void            
MultipacketReciever::RecievePacket( const void* packet_data, u32 packet_sz, u32 param )
{
    MultipacketHeader*  header = (MultipacketHeader*)packet_data;
    u8                  data[MaxMultipacketSize];

    if ( header->tag != NET_TAG_MERGED  &&  header->tag != NET_TAG_NONMERGED )
       return;

    Compressor.Decompress( data, sizeof(data), 
                           (u8*)packet_data+sizeof(MultipacketHeader), packet_sz-sizeof(MultipacketHeader) 
                         );

    #if NET_LOG_PACKETS
    Msg( "#receive multi-packet %u", packet_sz );
    #endif

    if( strstr( Core.Params,"-dump_traffic") ) 
    {
        static bool first_time  = true;
		FILE*       dump        = fopen( "raw-in-traffic.bins", (first_time)?"wb":"ab" );

        if( first_time )
        {
            fwrite( "BINS", 4, 1, dump );
            first_time = false;
        }

        u16 sz = header->unpacked_size;
        
        fwrite( &sz, sizeof(u16), 1, dump );
        fwrite( data, header->unpacked_size, 1, dump );
		fclose( dump );
    }


    bool    is_multi_packet = header->tag == NET_TAG_MERGED;
    u32     processed_sz    = 0;
    u8*     dat             = data;

    while( processed_sz < header->unpacked_size )
    {
        u32     size = (is_multi_packet)
                       ? u32(*((u16*)dat))
                       : header->unpacked_size;

        if( is_multi_packet )
            dat += sizeof(u16);

        #if NET_LOG_PACKETS
        Msg( "  packet %u", size );
        #endif


		//PrintParsedPacket("-- LL Receiving:", 1, dat, size);
        
		_Recieve( dat, size, param );

        dat          += size;
        processed_sz += size + ((is_multi_packet) ? sizeof(u16) : 0);
    }
}

void XRNETSERVER_API DumpNetCompressorStats(bool brief)
{
	Compressor.DumpStats(brief);
}

