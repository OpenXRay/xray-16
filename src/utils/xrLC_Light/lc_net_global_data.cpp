#include "stdafx.h"
#include "lc_net_global_data.h"
#include "xrlc_globaldata.h"
#include "file_compress.h"
// void  DataReadCreate( const char* fn );

// void decompress( const char* f_in_out );
void DataReadCreate(const char* fn) {}
namespace lc_net
{
void net_global_data_impl<gl_cl_data>::create_data_file(const char* path)
{
    FPU::m64r();
    Memory.mem_compact();
    // std::random_shuffle	(inlc_global_data()->g_deflectors().begin(),inlc_global_data()->g_deflectors().end());
    Logger.clMsg("create_global_data_write:  start");
    IWriter* file = FS.w_open(path);
    inlc_global_data()->write(*file);
    FS.w_close(file);
    compress(path);
    Logger.clMsg("create_global_data_write:  end");
    // inlc_global_data()->create_read_faces();
    // inlc_global_data()->create_write_faces();
}
bool net_global_data_impl<gl_cl_data>::create_data(const char* path)
{
    decompress(path);
    INetReaderFile r_global(path);
    // create_global_data();

    VERIFY(inlc_global_data());
    inlc_global_data()->read(r_global);

    FPU::m64r();
    Memory.mem_compact();
    // inlc_global_data()->create_read_faces();
    // inlc_global_data()->create_write_faces();
    return true;
}
}
