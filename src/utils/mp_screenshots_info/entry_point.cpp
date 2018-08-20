#include "pch.h"
#include "screenshots_writer.h"
#include "screenshots_reader.h"

void print_format() { printf("Format: mp_screenshots_info.exe [screenshot_file_name]\n"); };
void xrcore_log_cb(void* context, LPCSTR log_string) { printf("%s\n", log_string); };
u8* ss_buffer = NULL;
u32 ss_buffer_size = 0;
/*#ifdef DEBUG
void debug_sign_screenshot(IReader* screenshot)
{
    using namespace screenshots; //for writer
    int	tmp_jpeg_size			= screenshot->elapsed();
    if (tmp_jpeg_size < 0)
        return;
    u32	tmp_buff_size			= tmp_jpeg_size + writer::info_max_size;
    u8*	tmp_buff				= static_cast<u8*>(xr_malloc(tmp_buff_size));

    screenshot->r				(tmp_buff, tmp_jpeg_size);

    writer	tmp_writer			(tmp_buff, tmp_jpeg_size, tmp_buff_size);

    tmp_writer.set_player_name			("some_cheater");
    tmp_writer.set_player_cdkey_digest	("238940293847298374982734");
    u32 final_ss_size					= tmp_writer.write_info();

    IWriter* result_writer		= FS.w_open("$screenshots$", "result.jpg");
    VERIFY						(result_writer);
    result_writer->w			(tmp_buff, final_ss_size);
    FS.w_close					(result_writer);

    xr_free						(tmp_buff);
    Msg							("Screenshot signed successfully !");
};
#endif*/

void screenshot_info(IReader* screenshot)
{
    using namespace screenshots; // for reader
    reader tmp_reader(screenshot);
    if (!tmp_reader.is_valid())
    {
        Msg("ERROR: screenshot not valid or corrupted.");
        return;
    }
    Msg("Verifying screenshot digital sign...");
    bool verify_res = tmp_reader.verify();
    Msg("Screenshot verification: %s", verify_res ? "Succeeded" : "FAILED");
    if (!verify_res)
    {
        return;
    }
    Msg("Screenshot info:");
    Msg("	Player name:		%s", tmp_reader.player_name().c_str());
    Msg("	Player cdkey digest:	%s", tmp_reader.player_cdkey_digest().c_str());
    // Msg("	Admin name:		%s", tmp_reader.admin_name().c_str());
    Msg("	Creation date:		%s", tmp_reader.creation_date().c_str());
}

int main(int argc, char** argv)
{
    printf("Copyright (C) GSC Game World 2009\n");
    if (argc < 2)
    {
        printf("ERROR: bad parameters.\n");
        print_format();
        return EXIT_FAILURE;
    }
    printf("Initializing core...\n");
    Core.Initialize("mp_screenshots_info", nullptr, LogCallback(xrcore_log_cb, nullptr), TRUE, "fsgame4mpu.ltx");

#ifdef DEBUG
    if (strstr(argv[1], "--gen_params"))
    {
        crypto::xr_dsa::generate_params();
        Core._destroy();
        return EXIT_SUCCESS;
    }
#endif
    if (argc < 3)
    {
        printf("ERROR: screenshot file not specified.\n");
        print_format();
        return EXIT_FAILURE;
    }
    const char* ss_file = argv[2];
    IReader* tmp_jpg = FS.r_open("$screenshots$", ss_file);
    if (!tmp_jpg)
    {
        tmp_jpg = FS.r_open(ss_file);
        if (!tmp_jpg)
        {
            printf("ERROR: can't open file: %s", ss_file);
            Core._destroy();
            return EXIT_FAILURE;
        }
    }
    /*#ifdef DEBUG
        if (strstr(argv[1], "--sign"))
        {
            debug_sign_screenshot	(tmp_jpg);
        } else if (strstr(argv[1], "--info"))
        {
            screenshot_info			(tmp_jpg);
        }
    #else*/
    screenshot_info(tmp_jpg);

    FS.r_close(tmp_jpg);
    xr_free(ss_buffer);
    Core._destroy();
    return EXIT_SUCCESS;
}
