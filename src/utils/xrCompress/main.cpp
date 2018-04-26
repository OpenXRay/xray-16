#include "stdafx.h"
#include "xrCompress.h"

#ifndef MOD_COMPRESS
extern int ProcessDifference();
#endif

int __cdecl main(int argc, char* argv[])
{
    xrDebug::Initialize(false);
    Core.Initialize("xrCompress", 0, FALSE);
    printf(
        "\n\nXrCompressor (modifided "
        "LostAlphaRus)\n----------------------------------------------------------------------------");

    LPCSTR params = GetCommandLine();

    if (strstr(params, "-delete"))
    {
        // db
        remove("gamedata.db0");
        remove("gamedata.db1");
        remove("gamedata.db2");
        remove("gamedata.db3");
        remove("gamedata.db4");
        remove("gamedata.db5");
        remove("gamedata.db6");
        remove("gamedata.db7");
        remove("gamedata.db8");
        remove("gamedata.db9");
        remove("gamedata.db10");
        remove("gamedata.db11");
        remove("gamedata.db12");
        remove("gamedata.db13");
        remove("gamedata.db14");
        remove("gamedata.db15");
        remove("gamedata.db16");
        remove("gamedata.db17");
        remove("gamedata.db18");
        remove("gamedata.db19");
        remove("gamedata.db20");

        // xdb
        remove("gamedata.xdb0");
        remove("gamedata.xdb1");
        remove("gamedata.xdb2");
        remove("gamedata.xdb3");
        remove("gamedata.xdb4");
        remove("gamedata.xdb5");
        remove("gamedata.xdb6");
        remove("gamedata.xdb7");
        remove("gamedata.xdb8");
        remove("gamedata.xdb9");
        remove("gamedata.xdb10");
        remove("gamedata.xdb11");
        remove("gamedata.xdb12");
        remove("gamedata.xdb13");
        remove("gamedata.xdb14");
        remove("gamedata.xdb15");
        remove("gamedata.xdb16");
        remove("gamedata.xdb17");
        remove("gamedata.xdb18");
        remove("gamedata.xdb19");
        remove("gamedata.xdb20");
    }

    if (strstr(params, "-nodelete"))
    {
        printf("\n\nINFO: DB.ARHIVE the file is not deleted!!!");
    }

    xrCompressor C;
    C.SetStoreFiles(NULL != strstr(params, "-store"));

#ifndef MOD_COMPRESS
    if (strstr(params, "-diff"))
    {
        ProcessDifference();
    }
    else
#endif

#ifndef MOD_XDB
        if (strstr(params, "-pack"))
#endif
    {
#ifndef MOD_COMPRESS
        if (argc < 2)
        {
            printf("ERROR: u must pass folder name as parameter.\n");
            printf("-diff /? option to get information about creating difference.\n");
            printf("-fast	- fast compression.\n");
            printf("-store	- store files. No compression.\n");
            printf("-ltx <file_name.ltx> - pathes to compress.\n");
            printf("\n");
            printf("LTX format:\n");
            printf("	[config]\n");
            printf("	;<path>     = <recurse>\n");
            printf("	.\\         = false\n");
            printf("	textures    = true\n");

            Core._destroy();
            return 3;
        }
#endif

        string_path folder;
        strconcat(sizeof(folder), folder, argv[1], "\\");
        _strlwr_s(folder, sizeof(folder));
        printf("\nCompressing files (%s)...\n\n", folder);

        FS._initialize(CLocatorAPI::flTargetFolderOnly, folder);
        FS.append_path("$working_folder$", "", 0, false);

        C.SetFastMode(NULL != strstr(params, "-nocompress"));
        C.SetTargetName(argv[1]);

        LPCSTR p = strstr(params, "-ltx");

        if (0 != p)
        {
            string64 ltx_name;
            sscanf(strstr(params, "-ltx ") + 5, "%[^ ] ", ltx_name);

            CInifile ini(ltx_name);
            printf("Processing ...\n");
            C.ProcessLTX(ini);
        }
        else
        {
            string64 header_name;
            sscanf(strstr(params, "-header ") + 8, "%[^ ] ", header_name);
            C.SetPackHeaderName(header_name);
            C.ProcessTargetFolder();
        }
    }

    Core._destroy();
    return 0;
}
