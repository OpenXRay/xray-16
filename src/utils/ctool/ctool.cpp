

	#include <stdlib.h>
    #include "../../xrcore/_types.h"
	#undef FLT_MAX
	#undef FLT_MIN
	#include <stdio.h>
    #include <vector>
    #include <algorithm>
    #include <string>
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    using namespace std;


#define MAKE_FOUR_CC(ch0,ch1,ch2,ch3)   \
((u32)(u8)(ch0)            |  \
((u32)(u8)(ch1) << 8)      |  \
((u32)(u8)(ch2) << 16)     |  \
((u32)(u8)(ch3) << 24 ))      

inline bool
IsEmptyString( const char* str )
{
    return !(str  &&  str[0] != '\0' );
}

static unsigned _LZO_MinPacketSize = 8;
static unsigned _LZO_MaxPacketSize = 8*1024;

int	_PPM_MaxOrder	= 2;
int _PPM_SaSize		= 48;
int	_PPM_ModelSize	= 64;


//------------------------------------------------------------------------------

static void
_UnpackPackets( const char* src_bin, const char* dst_name="" )
{
    FILE*   src_file = fopen( src_bin, "rb" );

    if( src_file )
    {
        // read the file
        
        fseek( src_file, 0, SEEK_END );
    
        long    src_sz      = ftell( src_file );
        u8*     src_data    = new u8 [src_sz];

        fseek( src_file, 0, SEEK_SET );
        fread( src_data, src_sz, 1, src_file );
        fclose( src_file );


        // process parts

        u32 id = *((u32*)src_data);

        if( id == MAKE_FOUR_CC('B','I','N','S') )
        {
            const u8*   data        = src_data + sizeof(u32);
            const u8*   data_end    = src_data + src_sz;
            unsigned    count       = 0;
            char        bin_name[512];

            while( data < data_end )
            {
                _snprintf( bin_name, sizeof(bin_name)-1, "data-%08u.bin", count+1 );
            
                u16     sz  = *((u16*)data);
                FILE*   bin = fopen( bin_name, "wb" );

                data += sizeof(u16);
                fwrite( data, sz, 1, bin );
                fclose( bin );

                data += sz;
                ++count;
            }
        }
        else
        {
            // TODO: error 
        }

		delete[] src_data;
    }
    else
    {
        // TODO: error here
    }
}


//==============================================================================

struct
PacketInfo
{
                PacketInfo();
                ~PacketInfo();

    void        add_data( const u8* data, unsigned data_sz );
    void        remove_all_data();


    static bool WeightPredicate( const PacketInfo& p1, const PacketInfo& p2 )
                {
                    return p1.weight > p2.weight;
                }
    static bool SizePredicate( const PacketInfo& p1, const PacketInfo& p2 )
                {
                    return p1.size > p2.size;
                }

public:

    unsigned    size;
    unsigned    count;
    float       weight;

    struct
    Data
    {
        u8*         data;
        unsigned    size;
    };

    vector<Data>    packet;
};


//------------------------------------------------------------------------------

PacketInfo::PacketInfo()
  : size(0),
    count(0),
    weight(0)
{
}


//------------------------------------------------------------------------------

PacketInfo::~PacketInfo()
{
}


//------------------------------------------------------------------------------

void        
PacketInfo::add_data( const u8* data, unsigned data_sz )
{
    if( data_sz == size )
    {
        packet.push_back( Data() );

        packet.back().data = new u8[data_sz];
        packet.back().size = data_sz;
        memcpy( packet.back().data, data, data_sz );

        ++count;
    }
}


//------------------------------------------------------------------------------

void        
PacketInfo::remove_all_data()
{
    for( unsigned i=0; i<packet.size(); ++i )
        delete[] packet[i].data;
    packet.clear();
    count = 0;
}


//------------------------------------------------------------------------------
extern int MakePPMDictionaryFromFile(FILE* raw_bins_file_src);
extern int MakePPMDictionary(char const * file_name);

static void
_BuildDictionary( const char* bins_file, const char* dst_name="" )
{
	vector<PacketInfo>  packet_info;
    unsigned            total_packet_count = 0;
    
    FILE*   src_file = fopen( bins_file, "rb" );
	static char const * raw_bins_filename = "raw_bins_tmp.bin";
	FILE*	raw_bins_file = fopen(raw_bins_filename, "wb");
	if (!raw_bins_file)
	{
		if (src_file)
			fclose(src_file);
		printf("ERROR: failed to create raw_bins_tmp.bin");
		return;
	}

    if( src_file )
    {
		
        // read the file
        
        fseek( src_file, 0, SEEK_END );
    
        long    src_sz      = ftell( src_file );
        u8*     src_data    = new u8 [src_sz];

        fseek( src_file, 0, SEEK_SET );
        fread( src_data, src_sz, 1, src_file );
        fclose( src_file );


        // get packet data

        u32 id = *((u32*)src_data);

        if( id == MAKE_FOUR_CC('B','I','N','S') )
        {
            const u8*   data        = src_data + sizeof(u32);
            const u8*   data_end    = src_data + src_sz;
            unsigned    count       = 0;

            while( data < data_end )
            {
                u16     sz  = *((u16*)data);

                data += sizeof(u16);

                vector<PacketInfo>::iterator info;
                
                for( info=packet_info.begin(); info!=packet_info.end(); ++info )
                {
                    if( info->size == sz )
                        break;
                }
                if( info == packet_info.end() )
                {
                    packet_info.push_back( PacketInfo() );
                    info = packet_info.end()-1;

                    info->size  = sz;
                    info->count = 0;
                }

                info->add_data( data, sz );
				fwrite(data, sz, 1, raw_bins_file);
                
                data += sz;
                ++total_packet_count;
            }

        }
        else
        {
			printf("ERROR: Unknown file format");
        }

        delete[] src_data;
    }
    else
    {
		printf("ERROR: can't open file");
    }

	fclose(raw_bins_file);
	MakePPMDictionary(raw_bins_filename);

    // dump stats by size

    for( unsigned i=0; i<packet_info.size(); ++i )
    {
        packet_info[i].weight = float(packet_info[i].count) / float(total_packet_count);
    }

    sort( packet_info.begin(), packet_info.end(), PacketInfo::SizePredicate );

        
    printf( "\nsorted by size\n" );
    for( unsigned i=0; i<packet_info.size(); ++i )
    {
        printf( "%-6u : %-4u (%2.1f%%)\n", 
                packet_info[i].size, packet_info[i].count,
                packet_info[i].weight*100.0f
              );
    }


    // dump stats by frequency

    for( unsigned i=0; i<packet_info.size(); ++i )
    {
        packet_info[i].weight = float(packet_info[i].count) / float(total_packet_count);
    }

    sort( packet_info.begin(), packet_info.end(), PacketInfo::WeightPredicate );

        
    printf( "\nsorted by frequency\n" );

    unsigned    total_cnt = 0;
    
    for( unsigned i=0; i<packet_info.size(); ++i )
    {
        printf( "%-6u : %-4u (%2.1f%%)\n", 
                packet_info[i].size, packet_info[i].count,
                packet_info[i].weight*100.0f
              );

        total_cnt += packet_info[i].count;
    }
    printf( "total count = %u\n", total_cnt );


    // build dictionary

    const char* dic_file    = IsEmptyString(dst_name)  ? "lzo.dic"  : dst_name;
    FILE*       dic         = fopen( dic_file, "wb" );

	float const lzo_dict_max_size = 5.0f * 1024; //5 Kb

    if( dic )
    {
//        unsigned    min_sz  = 200;
//        unsigned    max_sz  = 350;
        unsigned    min_sz  = _LZO_MinPacketSize;
        unsigned    max_sz  = _LZO_MaxPacketSize;
    
        for( unsigned i=0; i<packet_info.size(); ++i )
        {
            const PacketInfo&   info = packet_info[i];

            if( info.size < min_sz  ||  info.size > max_sz )
                continue;

            
            /*unsigned    cnt = (info.size < 32) 
                              ? unsigned((10000.0f/float(info.size)) * info.weight)
                              : 1;*/
            
			unsigned int n   = 0;
			unsigned int cnt = unsigned int((lzo_dict_max_size*info.weight) / info.size);
           
            for( unsigned p=0,n=0; p<cnt; ++p,++n )
            {
                if( n >= info.count )
                    n = 0;
                    
                fwrite( info.packet[n].data, info.packet[n].size, 1, dic );
            }
        }
		
        fclose( dic );
    }
    else
    {
		printf("ERROR: failed to create LZO dic file");
    }
}


//==============================================================================

int
main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        printf( "usage: CTOOL <command> [option(s)] [file(s)]\n" );
        printf( "commands:\n" );
        printf( "u -- unpack packets from .bins-file\n" );
        printf( "d -- build dictionary from .bins-file\n" );
        return -1;
    }

    int     i = 2;
	string  dic_name;
    
    for( ; i<argc; ++i )
    {
        if( argv[i][0] != '-'  &&  argv[i][0] != '/' )
            break;

		if     ( !_strnicmp( argv[i]+1, "dic", 3 ) )
            dic_name = argv[i]+1+3+1;
        else if( !_strnicmp( argv[i]+1, "min_sz", 6 ) )
            _LZO_MinPacketSize = atoi( argv[i]+1+6+1 );
        else if( !_strnicmp( argv[i]+1, "max_sz", 6 ) )
            _LZO_MaxPacketSize = atoi( argv[i]+1+6+1 );
		else if (!_strnicmp( argv[i]+1, "order", 5 ) )
			_PPM_MaxOrder = atoi(argv[i]+1+5+1);
		else if (!_strnicmp( argv[i]+1, "salloc", 6 ) )
			_PPM_SaSize	= atoi(argv[i]+1+6+1);
		else if (!_strnicmp( argv[i]+1, "msize", 5 ) )
			_PPM_ModelSize = atoi(argv[i]+1+5+1);

        if( _LZO_MinPacketSize < 8 )
            _LZO_MinPacketSize = 8;
        
        if( _LZO_MaxPacketSize > 64*1024 )
            _LZO_MinPacketSize = 64*1024;
    }

    
    if( i < argc )
    {
        if( argv[1][0] == 'u' )
            _UnpackPackets( argv[i] );
        else if( argv[1][0] == 'd' )
            _BuildDictionary( argv[i], dic_name.c_str() );
    }

    return 0;
}



