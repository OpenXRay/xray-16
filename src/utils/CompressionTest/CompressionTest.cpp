#include <stdio.h>
#include <vector>
#include <algorithm>
#include "xrCore/Compression/PPMd.h"
#include "xrCore/Compression/compression_ppmd_stream.h"

#pragma warning(push)
#pragma warning(disable : 193 128 810)
#include "lzo/lzo1x.h"
#include "lzo/lzo1y.h"
#pragma warning(pop)

extern compression::ppmd::stream* trained_model;

typedef compression::ppmd::stream stream;
typedef unsigned char uint8_t;

using namespace std;

//==============================================================================

class StopWatch
{
public:
    StopWatch() { ::QueryPerformanceFrequency(&_freq); }
    void start() { ::QueryPerformanceCounter(&_start_time); }
    void stop() { ::QueryPerformanceCounter(&_stop_time); }
    double cur_time() const
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return ((t.QuadPart - _start_time.QuadPart) * 1000.0) / _freq.QuadPart;
    }
    double time() const // in milliseconds
    {
        return ((_stop_time.QuadPart - _start_time.QuadPart) * 1000.0) / _freq.QuadPart;
    }

private:
    LARGE_INTEGER _freq;
    LARGE_INTEGER _start_time;
    LARGE_INTEGER _stop_time;
};

//------------------------------------------------------------------------------

void _STDCALL PrintInfo(_PPMD_FILE*, _PPMD_FILE*) {}
//------------------------------------------------------------------------------

struct BlockInfo
{
    unsigned size;
    unsigned count;

    float min_time;
    float max_time;
    float avg_time;

    float min_ratio;
    float max_ratio;
    float avg_ratio;

    float total_time;
    float total_ratio;

    BlockInfo(unsigned sz = 0)
        : size(sz), count(0), min_time(1000.0f), max_time(0), avg_time(0), min_ratio(1.0f), max_ratio(0), avg_ratio(0),
          total_time(0), total_ratio(0)
    {
    }

    class EqualSize
    {
    public:
        EqualSize(unsigned sz) : _sz(sz) {}
        bool operator()(const BlockInfo& bi) { return bi.size == this->_sz; }
    private:
        unsigned _sz;
    };

    static bool GreaterSize(const BlockInfo& i1, const BlockInfo& i2) { return i1.size > i2.size; }
};

//------------------------------------------------------------------------------

static const char* _DefaultMdlName = "!PPMd.mdl";
char* _ModelData = NULL;
long _ModelDataSize = 0;

static const u32 _SuballocatorSize = 32;
static const u32 _OrderModel = 8;
static const MR_METHOD _RestorationMethodCutOff = MRM_FREEZE;
// static const MR_METHOD	_RestorationMethodCutOff	= MRM_RESTART;

static char* _LZOWrkMem = NULL;
static uint8_t* _LZO_Dict = NULL;
static unsigned _LZO_Dict_Sz = 0;
static const char* _DefaultDictName = "LZO.dic";

static vector<BlockInfo> _PPM_BlockInfo;
static vector<BlockInfo> _LZO_BlockInfo;
static unsigned _PPM_TotalUncompressed = 0;
static unsigned _PPM_TotalCompressed = 0;
static unsigned _LZO_TotalUncompressed = 0;
static unsigned _LZO_TotalCompressed = 0;

//------------------------------------------------------------------------------

static void _InitPPM(const char* model_file = 0)
{
    if (model_file)
    {
        FILE* mdl = fopen(model_file, "rb");

        if (mdl)
        {
            fseek(mdl, 0, SEEK_END);

            _ModelDataSize = ftell(mdl);
            _ModelData = new char[_ModelDataSize];
            trained_model = xr_new<compression::ppmd::stream>(_ModelData, _ModelDataSize);

            fseek(mdl, 0, SEEK_SET);
            fread(_ModelData, _ModelDataSize, 1, mdl);
            fclose(mdl);

            printf("using PPMd-model trained data \"%s\"\n", model_file);
        }
    }

    StartSubAllocator(_SuballocatorSize);
}

//------------------------------------------------------------------------------

static void _InitLZO(const char* dic_name = _DefaultDictName)
{
    lzo_init();
    _LZOWrkMem = new char[LZO1X_999_MEM_COMPRESS + 16];
    _LZOWrkMem = (char*)((std::uintptr_t)(_LZOWrkMem + 16) & (~(16 - 1)));

    FILE* dic = fopen(dic_name, "rb");

    if (dic)
    {
        fseek(dic, 0, SEEK_END);

        _LZO_Dict_Sz = ftell(dic);
        _LZO_Dict = new uint8_t[_LZO_Dict_Sz];

        fseek(dic, 0, SEEK_SET);
        fread(_LZO_Dict, _LZO_Dict_Sz, 1, dic);
        fclose(dic);

        printf("using LZO-dict \"%s\"\n", dic_name);
    }
}

//------------------------------------------------------------------------------
extern void save_dictionary();

static bool _ProcessFile_PPMd(const char* file_name)
{
    bool success = false;
    FILE* file = fopen(file_name, "rb");

    if (file)
    {
        // read data

        fseek(file, 0, SEEK_END);

        unsigned src_size = ftell(file);
        char* src_data = new char[src_size];

        fseek(file, 0, SEEK_SET);
        fread(src_data, src_size, 1, file);
        fclose(file);

        // compress it

        unsigned comp_size = src_size * 4;
        char* comp_data = new char[comp_size];
        stream src(src_data, src_size);
        stream dst(comp_data, comp_size);

        memset(comp_data, 0xCC, comp_size);

        StopWatch timer;

        timer.start();
        if (trained_model)
            trained_model->rewind();
        EncodeFile(&dst, &src, _OrderModel, _RestorationMethodCutOff);
        timer.stop();
        printf("PPMd1 :  %2.0f%% %1.5fms  %u->%u\n", 100.0f * float(dst.tell()) / float(src.tell()),
            (float)timer.time(), src.tell(), dst.tell());

        timer.start();
        if (trained_model)
            trained_model->rewind();
        EncodeFile(&dst, &src, _OrderModel, _RestorationMethodCutOff);
        timer.stop();
        printf("PPMd2 :  %2.0f%% %1.5fms  %u->%u\n", 100.0f * float(dst.tell()) / float(src.tell()),
            (float)timer.time(), src.tell(), dst.tell());

        timer.start();
        if (trained_model)
            trained_model->rewind();
        EncodeFile(&dst, &src, _OrderModel, _RestorationMethodCutOff);
        timer.stop();
        printf("PPMd3 :  %2.0f%% %1.5fms  %u->%u\n", 100.0f * float(dst.tell()) / float(src.tell()),
            (float)timer.time(), src.tell(), dst.tell());

        _PPM_TotalUncompressed += src.tell();
        _PPM_TotalCompressed += (dst.tell() < src.tell()) ? dst.tell() : src.tell();

        // decompress it

        unsigned uncomp_size = src_size * 4;
        char* uncomp_data = new char[uncomp_size];
        stream uncomp(uncomp_data, uncomp_size);

        memset(uncomp_data, 0xDD, uncomp_size);

        dst.rewind();
        if (trained_model)
            trained_model->rewind();
        DecodeFile(&uncomp, &dst, _OrderModel, _RestorationMethodCutOff);

        // compare

        bool ok = true;
        unsigned err_b = 0;

        for (const char *s = src_data, *u = uncomp_data; s != src_data + src_size; ++s, ++u)
        {
            if (*s != *u)
            {
                ok = false;
                err_b = s - src_data;
                break;
            }
        }
        if (ok)
            printf(" OK\n");
        else
            printf(" ERROR (#%u  %02X != %02X)\n", err_b, src_data[err_b] & 0xFF, uncomp_data[err_b] & 0xFF);

        success = ok;

        // update stats

        vector<BlockInfo>::iterator i =
            find_if(_PPM_BlockInfo.begin(), _PPM_BlockInfo.end(), BlockInfo::EqualSize(src_size));

        if (i == _PPM_BlockInfo.end())
        {
            _PPM_BlockInfo.push_back(BlockInfo(src_size));
            i = _PPM_BlockInfo.end() - 1;
        }

        ++i->count;
        i->total_time += (float)timer.time();
        i->total_ratio += 100.0f * float(dst.tell()) / float(src.tell());

        // clean-up

        delete[] uncomp_data;
        delete[] comp_data;
        delete[] src_data;
    } // if file open

    return success;
}

//------------------------------------------------------------------------------

static bool _ProcessFile_LZO(const char* file_name)
{
    bool success = false;
    FILE* file = fopen(file_name, "rb");

    if (file)
    {
        // read data

        fseek(file, 0, SEEK_END);

        unsigned src_size = ftell(file);
        uint8_t* src_data = new uint8_t[src_size];

        fseek(file, 0, SEEK_SET);
        fread(src_data, src_size, 1, file);
        fclose(file);

        // compress it

        lzo_uint comp_size = src_size * 4;
        uint8_t* comp_data = new uint8_t[comp_size];

        memset(comp_data, 0xCC, comp_size);

        StopWatch timer;

        timer.start();
        if (_LZO_Dict)
        {
            lzo1x_999_compress_dict(src_data, src_size, comp_data, &comp_size, _LZOWrkMem, _LZO_Dict, _LZO_Dict_Sz);
        }
        else
        {
            lzo1x_999_compress(src_data, src_size, comp_data, &comp_size, _LZOWrkMem);
        }
        timer.stop();
        printf("LZO  :  %2.0f%% %1.5fms  %u->%u", 100.0f * float(comp_size) / float(src_size), (float)timer.time(),
            (unsigned)src_size, (unsigned)comp_size);

        _LZO_TotalUncompressed += src_size;
        _LZO_TotalCompressed += (comp_size < src_size) ? comp_size : src_size;

        // decompress it

        lzo_uint uncomp_size = src_size * 4;
        uint8_t* uncomp_data = new uint8_t[uncomp_size];

        memset(uncomp_data, 0xDD, uncomp_size);
        if (_LZO_Dict)
        {
            lzo1x_decompress_dict_safe(comp_data, comp_size, uncomp_data, &uncomp_size, NULL, _LZO_Dict, _LZO_Dict_Sz);
        }
        else
        {
            lzo1x_decompress(comp_data, comp_size, uncomp_data, &uncomp_size, NULL);
        }

        // compare

        bool ok = true;
        unsigned err_b = 0;

        for (const uint8_t *s = src_data, *u = uncomp_data; s != src_data + src_size; ++s, ++u)
        {
            if (*s != *u)
            {
                ok = false;
                err_b = s - src_data;
                break;
            }
        }
        if (ok)
            printf(" OK\n");
        else
            printf(" ERROR (#%u  %02X != %02X)\n", err_b, src_data[err_b] & 0xFF, uncomp_data[err_b] & 0xFF);

        success = ok;

        // update stats

        vector<BlockInfo>::iterator i =
            find_if(_LZO_BlockInfo.begin(), _LZO_BlockInfo.end(), BlockInfo::EqualSize(src_size));

        if (i == _LZO_BlockInfo.end())
        {
            _LZO_BlockInfo.push_back(BlockInfo(src_size));
            i = _LZO_BlockInfo.end() - 1;
        }

        ++i->count;
        i->total_time += (float)timer.time();
        i->total_ratio += 100.0f * float(comp_size) / float(src_size);

        // clean-up

        delete[] uncomp_data;
        delete[] comp_data;
        delete[] src_data;
    } // if file open

    return success;
}

//------------------------------------------------------------------------------

static bool _ProcessFile(const char* file_name)
{
    printf("\n%s\n", file_name);
    return (_ProcessFile_PPMd(file_name) && _ProcessFile_LZO(file_name));
    //    return _ProcessFile_LZO( file_name );
}

//==============================================================================
//
//  entry-point here
//

int main(int argc, char* argv[])
{
    const char* src_name = (argc > 1) ? argv[1] : 0;
    const char* mdl_name = _DefaultMdlName;
    const char* dic_name = _DefaultDictName;

    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];

        if (arg[0] == '-' || arg[0] == '/')
        {
            if (!_strnicmp(arg + 1, "mdl", 3))
                mdl_name = arg + 1 + 3 + 1;
            else if (!_strnicmp(arg + 1, "dic", 3))
                dic_name = arg + 1 + 3 + 1;
        }
    }

    if (argc > 1)
    {
        _InitPPM(mdl_name);
        _InitLZO(dic_name);

        WIN32_FIND_DATA found_data;
        HANDLE handle = ::FindFirstFile(src_name, &found_data);

        // extract dir-part of wildcard

        char dir[MAX_PATH] = ".";
        const char* end = src_name + strlen(src_name) - 1;

        while (end > src_name)
        {
            if (*end == '\\' || *end == '/')
                break;

            --end;
        }

        if (*end == '\\' || *end == '/')
        {
            strncpy(dir, src_name, end - src_name);
            dir[end - src_name] = '\0';
        }

        // process files

        if (handle != reinterpret_cast<HANDLE>(INVALID_HANDLE_VALUE))
        {
            BOOL rv = TRUE;
            bool all_ok = true;

            while (rv)
            {
                if (!(found_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    char name[2048];

                    _snprintf(name, sizeof(name), "%s\\%s", dir, found_data.cFileName);
                    if (!_ProcessFile(name))
                    {
                        all_ok = false;
                        break;
                    }
                }

                rv = ::FindNextFile(handle, &found_data);
            }

            if (all_ok)
                printf("\nAll OK\n");
            else
                printf("\nERROR(S) ocurred\n");
        }
        ::FindClose(handle);

        // dump stats

        for (unsigned i = 0; i < _PPM_BlockInfo.size(); ++i)
        {
            _PPM_BlockInfo[i].avg_ratio = _PPM_BlockInfo[i].total_ratio / _PPM_BlockInfo[i].count;
            _PPM_BlockInfo[i].avg_time = _PPM_BlockInfo[i].total_time / _PPM_BlockInfo[i].count;
        }
        sort(_PPM_BlockInfo.begin(), _PPM_BlockInfo.end(), BlockInfo::GreaterSize);

        for (unsigned i = 0; i < _LZO_BlockInfo.size(); ++i)
        {
            _LZO_BlockInfo[i].avg_ratio = _LZO_BlockInfo[i].total_ratio / _LZO_BlockInfo[i].count;
            _LZO_BlockInfo[i].avg_time = _LZO_BlockInfo[i].total_time / _LZO_BlockInfo[i].count;
        }
        sort(_LZO_BlockInfo.begin(), _LZO_BlockInfo.end(), BlockInfo::GreaterSize);

        printf("\n==========\nstats:\n");
        for (unsigned i = 0; i < _PPM_BlockInfo.size(); ++i)
        {
            //            R_ASSERT(_PPM_BlockInfo[i].size == _LZO_BlockInfo[i].size);

            printf("\n%u\n", _PPM_BlockInfo[i].size);
            printf("PPM :  %2.1f%%  %-2.3fms\n", _PPM_BlockInfo[i].avg_ratio, _PPM_BlockInfo[i].avg_time);
            printf("LZO :  %2.1f%%  %-2.3fms\n", _LZO_BlockInfo[i].avg_ratio, _LZO_BlockInfo[i].avg_time);
            /*
                        printf( "%-6u :  %2.1f%%  %-2.3fms\n",
                                _PPM_BlockInfo[i].size,
                                _PPM_BlockInfo[i].avg_ratio, _PPM_BlockInfo[i].avg_time
                              );
            */
        }

        printf("\nTOTAL\n");
        printf("PPMd :  %2.1f%%  %u -> %u\n", 100.0f * float(_PPM_TotalCompressed) / float(_PPM_TotalUncompressed),
            _PPM_TotalUncompressed, _PPM_TotalCompressed);
        printf("LZO  :  %2.1f%%  %u -> %u\n", 100.0f * float(_LZO_TotalCompressed) / float(_LZO_TotalUncompressed),
            _LZO_TotalUncompressed, _LZO_TotalCompressed);

    } // if( argc > 1 )

    return 0;
}
