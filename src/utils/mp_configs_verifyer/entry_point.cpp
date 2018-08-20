#include "pch.h"
#include "configs_dump_verifyer.h"

static char const* help_msg =
    "Format: mp_configs_verifyer.exe [--file | --unpack | --io_filter | --help] [file name]\n"
    "Keys:\n"
    "	--file, -f		Checks file <file_name> only\n"
    "	--unpack, -u		Unpacks packet file <file_name>\n"
    "	--io_filter, -i		Starts as filter, (stdin receives file names)\n"
    "	--help, -h		Prints this message\n";

void print_format() { printf(help_msg); };
void xrcore_log_cb(void* context, LPCSTR log_string) { printf("%s\n", log_string); };
void safe_verify(
    LPCSTR file_name, mp_anticheat::configs_verifyer& verifyer, u8* data, u32 const data_size, string256& dst_reason)
{
    __try
    {
        if (!verifyer.verify(data, data_size, dst_reason))
        {
            printf("CHEATER (%s): %s\n", file_name, dst_reason);
        }
        else
        {
            printf("GOOD\n");
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        printf("FATAL ERROR: failed to verify data\n");
    }
}

void check_file(LPCSTR file_name)
{
    mp_anticheat::configs_verifyer tmp_verifyer;
    IReader* tmp_reader = FS.r_open(file_name);
    if (!tmp_reader)
    {
        FS.rescan_path("$screenshots$", TRUE);
        tmp_reader = FS.r_open("$screenshots$", file_name);
        if (!tmp_reader)
        {
            printf("ERROR: can't open file (%s)\n", file_name);
            return;
        }
    }
    u32 data_size = tmp_reader->length();
    u8* data = static_cast<u8*>(xr_malloc(data_size + 1));

    tmp_reader->r(data, static_cast<int>(data_size));
    data[data_size] = 0;
    string256 dst_reason;
    dst_reason[0] = 0;
    safe_verify(file_name, tmp_verifyer, data, data_size, dst_reason);
    xr_free(data);
    FS.r_close(tmp_reader);
}

void create_unpack_name(string_path& dst_src_path)
{
    char* ext_pos = strstr(dst_src_path, ".cltx");
    if (ext_pos)
    {
        xr_strcpy(ext_pos, xr_strlen(ext_pos), ".ltx");
        return;
    }
    xr_strcat(dst_src_path, ".ltx");
};

static u32 const max_uncompressed_size = 0x100000; // 1 Mb
void unpack_file(LPCSTR file_name)
{
    mp_anticheat::configs_verifyer tmp_verifyer;
    string_path new_file_name;
    IReader* tmp_reader = FS.r_open(file_name);
    if (!tmp_reader)
    {
        tmp_reader = FS.r_open("$screenshots$", file_name);
        if (!tmp_reader)
        {
            printf("ERROR: can't open file (%s)\n", file_name);
            return;
        }
        FS.update_path(new_file_name, "$screenshots$", file_name);
    }
    else
    {
        xr_strcpy(new_file_name, file_name);
    }
    create_unpack_name(new_file_name);

    u32 data_size = tmp_reader->length() - sizeof(u32); // first word is unpacket size ...
    u8* data = static_cast<u8*>(xr_malloc(data_size));
    u32 uncomp_size = tmp_reader->r_u32();
    tmp_reader->r(data, data_size);

    if (uncomp_size > max_uncompressed_size)
    {
        printf("ERROR: bad archive\n");
        xr_free(data);
        FS.r_close(tmp_reader);
        return;
    }
    u8* uncomp_data = static_cast<u8*>(xr_malloc(uncomp_size));
    ppmd_yield_callback_t tmp_cb;

    u32 original_size = ppmd_decompress_mt(uncomp_data, uncomp_size, data, data_size, tmp_cb);

    if (uncomp_size != original_size)
    {
        printf("ERROR: bad archive (original size and unpacked size are not equal)\n");
        xr_free(uncomp_data);
        xr_free(data);
        FS.r_close(tmp_reader);
        return;
    }
    IWriter* res_writer = FS.w_open(new_file_name);
    if (!res_writer)
    {
        printf("ERROR: can't create result file (%s)\n", new_file_name);
        xr_free(uncomp_data);
        xr_free(data);
        FS.r_close(tmp_reader);
        return;
    }

    res_writer->w(uncomp_data, original_size);
    FS.w_close(res_writer);
    printf("Done\n");

    xr_free(uncomp_data);
    xr_free(data);
    FS.r_close(tmp_reader);
}

void run_configs_verifyer_server()
{
    string_path file_to_check;
    file_to_check[0] = 0;
    while (scanf_s("%s", file_to_check, sizeof(file_to_check)) == 1)
    {
        check_file(file_to_check);
    }
};

void initialize_core()
{
    Core.Initialize("mp_configs_info", nullptr, LogCallback(xrcore_log_cb, nullptr), TRUE, "fsgame4mpu.ltx");

    string_path fname;
    FS.update_path(fname, "$game_config$", "system.ltx");
    pSettings = new CInifile(fname, TRUE);
}
void deinitialize_core()
{
    CInifile** s = (CInifile**)(&pSettings);
    xr_delete(*s);
    Core._destroy();
}

int main(int argc, char** argv)
{
    printf("Copyright (C) GSC Game World 2009\n");
    if (argc <= 1)
    {
        printf("ERROR: bad command parameters\n");
        print_format();
        return EXIT_FAILURE;
    }

    LPCSTR file_name = argv[argc - 1];
    LPCSTR cmd_params = "";
    for (int i = 1; i < argc; ++i)
    {
        STRCONCAT(cmd_params, cmd_params, " ", argv[i]);
    }

    if (strstr(cmd_params, "--file") || strstr(cmd_params, " -f "))
    {
        printf("Initializing core...\n");
        initialize_core();
        check_file(file_name);
        deinitialize_core();
    }
    else if (strstr(cmd_params, "--unpack") || strstr(cmd_params, " -u "))
    {
        printf("Initializing core...\n");
        initialize_core();
        unpack_file(file_name);
        deinitialize_core();
    }
    else if (strstr(cmd_params, "--io_filter") || strstr(cmd_params, " -i"))
    {
        initialize_core();
        run_configs_verifyer_server();
        deinitialize_core();
    }
    else if (strstr(cmd_params, "--help") || strstr(cmd_params, " -h"))
    {
        print_format();
    }
#ifdef DEBUG
    else if (strstr(cmd_params, "--gen_params"))
    {
        printf("Initializing core...\n");
        initialize_core();
        crypto::xr_dsa::generate_params();
        deinitialize_core();
    }
#endif
    else
    {
        printf("ERROR: bad command parameters\n");
        print_format();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
