/****************************************************************************************

Copyright (C) NVIDIA Corporation 2003

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

*****************************************************************************************/



#include "nvDXT.h"

/*#ifdef _DEBUG
#include <Windows.h> 
#include "MemLeakDetect.h" 

CMemLeakDetect myMemLeakDetect; 
#endif
*/


HFILE filein = 0;
HFILE fileout;









char * supported_extensions[] =
{
    "*.psd",
    "*.png",
    "*.ppm",
    "*.tga",
    "*.bmp",
    "*.gif",
    "*.pgm",
    "*.ppm",
    "*.jpg",
    "*.jpeg",
    "*.lbm",
    "*.vid",
    "*.kps",
    "*.xbm",
    "*.tif",
    "*.tiff",
    "*.cel",
    "*.dds",
    "*.spr",
    "*.psg",
    "*.pcx",
    0,
};









char * PopupDirectoryRequestor(const char * initialdir, const char * title)
{
    static char temp[MAX_PATH_NAME];
    static char dir[MAX_PATH_NAME];

    BROWSEINFO bi;
    LPITEMIDLIST list;

    //HINSTANCE hInstance = GetModuleHandle(NULL);
    //gd3dApp->Create( hInstance  );

    bi.hwndOwner = 0; 
    bi.pidlRoot = 0; 
    bi.pszDisplayName = dir; 
    bi.lpszTitle = title; 
    bi.ulFlags = 0; 
    bi.lpfn = 0; 
    bi.lParam = 0; 
    bi.iImage = 0; 

    list = SHBrowseForFolder(&bi);

    if (list == 0)
        return 0;

    SHGetPathFromIDList(list, dir);

    return dir;
}




HRESULT nvDXT::compress_all( )
{

    struct _finddata_t image_file_data;
    long hFile;


    std::vector<std::string> image_files;

    int i = 0;
    int j;
    while(supported_extensions[i])
    {
        std::string fullwildcard = input_dirname;
        fullwildcard += "\\";
        fullwildcard += supported_extensions[i];

        hFile = _findfirst( fullwildcard.c_str(), &image_file_data );
        if (hFile != -1)
        {

            image_files.push_back(image_file_data.name);
            //compress_image_file(filetga.name, TextureFormat);


            // Find the rest of the .image files 
            while( _findnext( hFile, &image_file_data ) == 0 )
            {
                //compress_image_file(filetga.name, TextureFormat);
                image_files.push_back(image_file_data.name);
            }

            _findclose( hFile );
        }
        i++;
    }


    // remove .dds if other name exists

    int n = image_files.size();
    int * tags = new int[n];
    for(i=0; i<n; i++)
        tags[i] = 0;

    // for every .dds file, see if there is an existing non dds file
    for(i=0; i<n; i++)
    {

        std::string filename  = image_files[i];

        int pos = find_string(filename, ".dds");
        // found a .dds file
        if (pos != -1)
        {

            std::string prefix;
            prefix = filename.substr(0, pos);

            // search all files for prefix
            for(j=0; j<n; j++)
            {
                // .. don't match self
                if (i == j)
                    continue;

                // get second file
                int pos2 = find_string(image_files[j], ".dds");

                // if not a .dds file
                if (pos2 == -1)
                {
                    // is this the same prefix?
                    pos2 = find_string(image_files[j], const_cast<char *>(prefix.c_str()));
                    if (pos2 == 0)
                    {
                        tags[i] = 1;
                        break;
                    }

                }
            }
        }
    }


    HRESULT ahr = 0;
    for(i=0; i<n; i++)
    {
        if (tags[i] == 0)
        {
            std::string filename  = image_files[i];
            HRESULT hr = compress_image_file(const_cast<char *>(filename.c_str()), TextureFormat);
            if (hr)
                ahr = hr; // last error
        }
    }

    delete [] tags;

    return ahr;

}


void nvDXT::compress_recursive(const char * dirname, HRESULT & ahr)
{


    input_dirname = dirname;

    printf("Processing %s\n", dirname);
    HRESULT hr = ProcessDirectory();
    if (hr)
        ahr = hr;


    std::string fullname = dirname;
    fullname += "\\*.*";




    //char *wildcard = "*.*";

    struct _finddata_t image_file_data;
    long hFile;


    std::vector<std::string>  image_files;


    hFile = _findfirst( fullname.c_str(), &image_file_data );

    if (hFile != -1 && image_file_data.attrib & _A_SUBDIR)
    {
        if (image_file_data.name[0] != '.')
        {
            std::string subname = dirname;
            subname += "\\";
            subname += image_file_data.name; 

            compress_recursive(subname.c_str(), ahr);
        }
    }





    if (hFile != -1)
    {

        image_files.push_back(image_file_data.name);
        //compress_image_file(filetga.name, TextureFormat);


        // Find the rest of the .image files 
        while( _findnext( hFile, &image_file_data ) == 0 )
        {
            //compress_image_file(filetga.name, TextureFormat);
            image_files.push_back(image_file_data.name);

            if (image_file_data.attrib & _A_SUBDIR)//Subdirectory. Value: 0x10
            {
                if (image_file_data.name[0] != '.')
                {
                    std::string subname = dirname;
                    subname += "\\";
                    subname += image_file_data.name; 

                    compress_recursive(subname.c_str(), ahr);
                }

            }


        }

        _findclose( hFile );
    }

}



bool nvDXT::ProcessFileName(char * wildcard, std::string &fullwildcard, std::string & dirname)
{
    std::string temp;

    fullwildcard = "";

    if (strstr(wildcard, ":\\") || wildcard[0] == '\\')
    {
        temp = wildcard;
        int pos = temp.find_last_of("\\", strlen(wildcard));
        if (pos >= 0)
            dirname = temp.substr(0, pos);
        fullwildcard = wildcard;

        input_dirname = dirname;	
        return true;
    }
    else if (wildcard[0] == '//')
    {
        temp = wildcard;
        int pos = temp.find_last_of("//", strlen(wildcard));
        if (pos >= 0)
            dirname = temp.substr(0, pos);
        fullwildcard = wildcard;

        input_dirname = dirname;		
        return true;
    }
    else if (strrchr(wildcard, '\\'))
    {
        temp = wildcard;
        int pos = temp.find_last_of("\\", strlen(wildcard));
        if (pos >= 0)
            dirname = temp.substr(0, pos);

        fullwildcard = input_dirname;
        fullwildcard += "\\";
        fullwildcard += wildcard;


        input_dirname = dirname;
        return true;

    }
    else if (strrchr(wildcard, '//'))
    {
        temp = wildcard;
        int pos = temp.find_last_of("//", strlen(wildcard));
        if (pos >= 0)
            dirname = temp.substr(0, pos);

        fullwildcard = input_dirname;
        fullwildcard += "\\";
        fullwildcard += wildcard;

        input_dirname = dirname;
        return true;
    }
    else
    {
        fullwildcard = input_dirname;
        fullwildcard += "\\";
        fullwildcard += wildcard;
        return false;
    }

}

HRESULT nvDXT::compress_some(char * wildcard)
{


    int i;
    struct _finddata_t image_file_data;
    long hFile;


    std::vector<std::string>  image_files;

    int j;
    // Find first .c file in current directory 

    std::string fullwildcard;
    std::string dirname;

    ProcessFileName(wildcard, fullwildcard, dirname);


    std::string name;
    hFile = _findfirst( fullwildcard.c_str(), &image_file_data );
    if (hFile != -1)
    {

        // pre-pend directory to name
        //

        name = dirname;
        name += "\\";
        name += image_file_data.name;
        image_files.push_back(name);
        //compress_image_file(filetga.name, TextureFormat);


        // Find the rest of the .image files 
        while( _findnext( hFile, &image_file_data ) == 0 )
        {
            //compress_image_file(filetga.name, TextureFormat);

            name = dirname;
            name += "\\";
            name += image_file_data.name;

            image_files.push_back(name);
        }

        _findclose( hFile );
    }


    // remove .dds if other name exists

    int n = image_files.size();
    int * tags = new int[n];
    for(i=0; i<n; i++)
        tags[i] = 0;

    // for every .dds file, see if there is an existing non dds file
    for(i=0; i<n; i++)
    {

        std::string filename  = image_files[i];

        int pos = find_string(filename, ".dds");
        // found a .dds file
        if (pos != -1)
        {

            std::string prefix;
            prefix = filename.substr(0, pos);

            // search all files for prefix
            for(j=0; j<n; j++)
            {
                // .. don't match self
                if (i == j)
                    continue;

                // get second file
                int pos2 = find_string(image_files[j], ".dds");

                // if not a .dds file
                if (pos2 == -1)
                {
                    pos2 = find_string(image_files[j], const_cast<char *>(prefix.c_str()));
                    if (pos2 == 0)
                    {
                        tags[i] = 1;
                        break;
                    }

                }
            }
        }
    }


    HRESULT ahr = 0;

    for(i=0; i<n; i++)
    {
        if (tags[i] == 0)
        {
            std::string filename  = image_files[i];

            HRESULT hr = compress_image_file(const_cast<char *>(filename.c_str()), TextureFormat);
            if (hr)
                ahr = hr;
        }
    }

    delete [] tags;

    return ahr;


}


HRESULT nvDXT::compress_list( )
{

    FILE *fp = fopen( listfile.c_str(), "r");

    if (fp == 0)
    {
        error("Can't open list file <%s>\n", listfile.c_str());
        return ERROR_BAD_LIST_FILE;
    }

    HRESULT ahr = 0;
    char buff[MAX_PATH_NAME];
    while(fgets(buff, MAX_PATH_NAME, fp))
    {      
        // has a crlf at the end
        int t = strlen(buff);
        buff[t - 1] = 0;




        std::string fullname;
        std::string dirname;


        std::string save_input_dirname = input_dirname;


        bFullPathSpecified = ProcessFileName(buff, fullname, dirname);



        HRESULT hr = compress_image_file(const_cast<char *>(fullname.c_str()), TextureFormat);
        if (hr)
            ahr = hr;


        input_dirname = save_input_dirname;

    }

    fclose(fp);

    return ahr;

}





void usage()
{
    fprintf(stdout,"NVDXT\n");
    fprintf(stdout,"This program\n");
    fprintf(stdout,"   compresses images\n");
    fprintf(stdout,"   creates normal maps from color or alpha\n");
    fprintf(stdout,"   creates DuDv map\n");
    fprintf(stdout,"   creates cube maps\n");
    fprintf(stdout,"   writes out .dds file\n");
    fprintf(stdout,"   does batch processing\n");
    fprintf(stdout,"   reads .tga, .bmp, .gif, .ppm, .jpg, .tif, .cel, .dds, .png and .psd\n");
    fprintf(stdout,"   filters MIP maps\n");
    fprintf(stdout,"\n");
    fprintf(stdout,"Options:\n");
    fprintf(stdout,"  -profile <profile name> : Read a profile created from the Photoshop plugin\n");
    fprintf(stdout,"  -quick : use fast compression method\n");
    fprintf(stdout,"  -prescale <int> <int>: rescale image to this size first\n");
    fprintf(stdout,"  -rescale <nearest | hi | lo | next_lo>: rescale image to nearest, next highest or next lowest power of two\n");
    fprintf(stdout,"  -rel_scale <float, float> : relative scale of original image. 0.5 is half size Default 1.0, 1.0\n");

    fprintf(stdout,"Optional Filtering for rescaling. Default cube filter:\n");

    fprintf(stdout,"  -RescalePoint\n"); 
    fprintf(stdout,"  -RescaleBox\n");
    fprintf(stdout,"  -RescaleTriangle\n");
    fprintf(stdout,"  -RescaleQuadratic\n");
    fprintf(stdout,"  -RescaleCubic\n");

    fprintf(stdout,"  -RescaleCatrom\n");
    fprintf(stdout,"  -RescaleMitchell\n");

    fprintf(stdout,"  -RescaleGaussian\n");
    fprintf(stdout,"  -RescaleSinc\n");
    fprintf(stdout,"  -RescaleBessel\n");

    fprintf(stdout,"  -RescaleHanning\n");
    fprintf(stdout,"  -RescaleHamming\n");
    fprintf(stdout,"  -RescaleBlackman\n");
    fprintf(stdout,"  -RescaleKaiser\n");


    fprintf(stdout,"  -clamp <int, int> : maximum image size. image width and height are clamped\n");
    fprintf(stdout,"  -clampScale <int, int> : maximum image size. image width and height are scaled \n");
    
    fprintf(stdout,"  -window <left, top, right, bottom> : window of original window to compress\n");

    fprintf(stdout,"  -nomipmap : don't generate MIP maps\n");
    fprintf(stdout,"  -nmips <int> : specify the number of MIP maps to generate\n");

    fprintf(stdout,"  -rgbe : Image is RGBE format\n");
    fprintf(stdout,"  -dither : add dithering\n");



    fprintf(stdout,"  -sharpenMethod <method>: sharpen method MIP maps\n");
    fprintf(stdout,"  <method> is \n");
    fprintf(stdout,"        None\n");
    fprintf(stdout,"        Negative\n");
    fprintf(stdout,"        Lighter\n");
    fprintf(stdout,"        Darker\n");
    fprintf(stdout,"        ContrastMore\n");
    fprintf(stdout,"        ContrastLess\n");
    fprintf(stdout,"        Smoothen\n");
    fprintf(stdout,"        SharpenSoft\n");
    fprintf(stdout,"        SharpenMedium\n");
    fprintf(stdout,"        SharpenStrong\n");
    fprintf(stdout,"        FindEdges\n");
    fprintf(stdout,"        Contour\n");
    fprintf(stdout,"        EdgeDetect\n");
    fprintf(stdout,"        EdgeDetectSoft\n");
    fprintf(stdout,"        Emboss\n");
    fprintf(stdout,"        MeanRemoval\n");
    fprintf(stdout,"        UnSharp <radius, amount, threshold>\n");
    fprintf(stdout,"        XSharpen <xsharpen_strength, xsharpen_threshold>\n");
    //fprintf(stdout,"        WarpSharp\n");
    fprintf(stdout,"        Custom\n");




    fprintf(stdout,"  -pause : wait for keyboard on error\n");
    fprintf(stdout,"  -volume <filename> : create volume texture <filename>. Files specified with -list option\n");
    fprintf(stdout,"  -flip : flip top to bottom \n");
    fprintf(stdout,"  -timestamp : Update only changed files\n");
    fprintf(stdout,"  -list <filename> : list of files to convert\n");
    fprintf(stdout,"  -cubemap <filename> : create cube map <filename>. Files specified with -list option\n");
    fprintf(stdout,"                        positive x, negative x, positive y, negative y, positive z, negative z\n");
    fprintf(stdout,"  -all : all image files in current directory\n");
    fprintf(stdout,"  -outdir <directory>: output directory\n");
    fprintf(stdout,"  -deep [directory]: include all subdirectories\n");
    fprintf(stdout,"  -outsamedir : output directory same as input\n");
    fprintf(stdout,"  -overwrite : if input is .dds file, overwrite old file\n");
    fprintf(stdout,"  -forcewrite : write over readonly files\n");

    fprintf(stdout,"  -file <filename> : input file to process. Accepts wild cards\n");
    fprintf(stdout,"  -output <filename> : filename to write to\n");
    fprintf(stdout,"  -append <filename_append> : append this string to output filename\n");
    fprintf(stdout,"  -24 <dxt1c | dxt1a | dxt3 | dxt5 | u1555 | u4444 | u565 | u8888 | u888 | u555> : compress 24 bit images with this format\n");
    fprintf(stdout,"  -32 <dxt1c | dxt1a | dxt3 | dxt5 | u1555 | u4444 | u565 | u8888 | u888 | u555> : compress 32 bit images with this format\n");
    fprintf(stdout,"\n");

    fprintf(stdout,"  -swap : swap rgb\n");
    fprintf(stdout,"  -gamma <float value>: gamma correcting during filtering\n");
    fprintf(stdout,"  -binaryalpha : treat alpha as 0 or 1\n");
    fprintf(stdout,"  -alpha_threshold <byte>: [0-255] alpha reference value \n");
    fprintf(stdout,"  -alphaborder : border images with alpha = 0\n");
    fprintf(stdout,"  -alphaborderLeft : border images with alpha (left) = 0\n");
    fprintf(stdout,"  -alphaborderRight : border images with alpha (right)= 0\n");
    fprintf(stdout,"  -alphaborderTop : border images with alpha (top) = 0\n");
    fprintf(stdout,"  -alphaborderBottom : border images with alpha (bottom)= 0\n");

    fprintf(stdout,"  -fadeamount <int>: percentage to fade each MIP level. Default 15\n");

    fprintf(stdout,"  -fadecolor : fade map (color, normal or DuDv) over MIP levels\n");
    fprintf(stdout,"  -fadetocolor <hex color> : color to fade to\n");

    fprintf(stdout,"  -fadealpha : fade alpha over MIP levels\n");
    fprintf(stdout,"  -fadetoalpha <byte>: [0-255] alpha to fade to\n");

    fprintf(stdout,"  -border : border images with color\n");
    fprintf(stdout,"  -bordercolor <hex color> : color for border\n");
    fprintf(stdout,"  -force4 : force DXT1c to use always four colors\n");


    fprintf(stdout,"\n");



    fprintf(stdout,"Texture Format  Default DXT3:\n");
    fprintf(stdout,"  -dxt1c  : DXT1 (color only)\n");
    fprintf(stdout,"  -dxt1a  : DXT1 (one bit alpha)\n");
    fprintf(stdout,"  -dxt3   : DXT3\n");
    fprintf(stdout,"  -dxt5   : DXT5\n\n");
    fprintf(stdout,"  -u1555  : uncompressed 1:5:5:5\n");
    fprintf(stdout,"  -u4444  : uncompressed 4:4:4:4\n");
    fprintf(stdout,"  -u565   : uncompressed 5:6:5\n");
    fprintf(stdout,"  -u8888  : uncompressed 8:8:8:8\n");
    fprintf(stdout,"  -u888   : uncompressed 0:8:8:8\n");
    fprintf(stdout,"  -u555   : uncompressed 0:5:5:5\n");
    fprintf(stdout,"  -p8     : paletted 8 bit (256 colors)\n");
    fprintf(stdout,"  -p4     : paletted 4 bit (16 colors)\n");
    fprintf(stdout,"  -a8     : 8 bit alpha channel\n");
    fprintf(stdout,"  -cxv8u8 : normal map format\n");
    fprintf(stdout,"  -v8u8   : EMBM format (two component signed)\n");
    fprintf(stdout,"  -A8L8   : 8 bit alpha channel, 8 bit luminance\n");
    fprintf(stdout,"  -fp32x4 : fp32 four channels (A32B32G32R32F)\n");
    fprintf(stdout,"  -fp32   : fp32 one channel (R32F)\n");
    fprintf(stdout,"  -fp16x4 : fp16 four channels (A16B16G16R16F)\n");

    fprintf(stdout,"\n");

    fprintf(stdout,"Optional Mip Map Filtering. Default box filter:\n");

    fprintf(stdout,"  -Point\n"); 
    fprintf(stdout,"  -Box\n");
    fprintf(stdout,"  -Triangle\n");
    fprintf(stdout,"  -Quadratic\n");
    fprintf(stdout,"  -Cubic\n");

    fprintf(stdout,"  -Catrom\n");
    fprintf(stdout,"  -Mitchell\n");

    fprintf(stdout,"  -Gaussian\n");
    fprintf(stdout,"  -Sinc\n");
    fprintf(stdout,"  -Bessel\n");

    fprintf(stdout,"  -Hanning\n");
    fprintf(stdout,"  -Hamming\n");
    fprintf(stdout,"  -Blackman\n");
    fprintf(stdout,"  -Kaiser\n");

    fprintf(stdout,"\n");



    fprintf(stdout,"***************************\n");
    fprintf(stdout,"To make a normal or dudv map, specify one of\n");
    fprintf(stdout,"  -n4 : normal map 4 sample\n");
    fprintf(stdout,"  -n3x3 : normal map 3x3 filter\n");
    fprintf(stdout,"  -n5x5 : normal map 5x5 filter\n");
    fprintf(stdout,"  -n7x7 : normal map 7x7 filter\n");
    fprintf(stdout,"  -n9x9 : normal map 9x9 filter\n");
    fprintf(stdout,"  -dudv : DuDv\n");

    fprintf(stdout,"\n");


    fprintf(stdout,"and source of height info:\n");
    fprintf(stdout,"  -alpha : alpha channel\n");
    fprintf(stdout,"  -rgb : average rgb\n");
    fprintf(stdout,"  -biased : average rgb biased\n");
    fprintf(stdout,"  -red : red channel\n");
    fprintf(stdout,"  -green : green channel\n");
    fprintf(stdout,"  -blue : blue channel\n");
    fprintf(stdout,"  -max : max of (r,g,b)\n");
    fprintf(stdout,"  -colorspace : mix of r,g,b\n");
    fprintf(stdout,"  -norm : normalize mip maps (source is a normal map)\n\n");
    fprintf(stdout,"-signed : signed output for normal maps\n");

    fprintf(stdout,"\n");

    fprintf(stdout,"Normal/DuDv Map dxt:\n");
    fprintf(stdout,"  -aheight : store calculated height in alpha field\n");
    fprintf(stdout,"  -aclear : clear alpha channel\n");
    fprintf(stdout,"  -awhite : set alpha channel = 1.0\n");
    fprintf(stdout,"  -scale <float> : scale of height map. Default 1.0\n");
    fprintf(stdout,"  -wrap : wrap texture around. Default off\n");
    fprintf(stdout,"  -minz <int> : minimum value for up vector [0-255]. Default 0\n\n");


    fprintf(stdout,"***************************\n");
    fprintf(stdout,"To make a depth sprite, specify:");
    fprintf(stdout,"\n");
    fprintf(stdout,"  -depth\n");
    fprintf(stdout,"\n");
    fprintf(stdout,"and source of depth info:\n");
    fprintf(stdout,"  -alpha  : alpha channel\n");
    fprintf(stdout,"  -rgb    : average rgb (default)\n");
    fprintf(stdout,"  -red    : red channel\n");
    fprintf(stdout,"  -green  : green channel\n");
    fprintf(stdout,"  -blue   : blue channel\n");
    fprintf(stdout,"  -max    : max of (r,g,b)\n");
    fprintf(stdout,"  -colorspace : mix of r,g,b\n");
    fprintf(stdout,"\n");
    fprintf(stdout,"Depth Sprite dxt:\n");
    fprintf(stdout,"  -aheight : store calculated depth in alpha channel\n");
    fprintf(stdout,"  -aclear : store 0.0 in alpha channel\n");
    fprintf(stdout,"  -awhite : store 1.0 in alpha channel\n");
    fprintf(stdout,"  -scale <float> : scale of depth sprite (default 1.0)\n");
    fprintf(stdout,"  -alpha_modulate : multiplies color by alpha during filtering\n");
    fprintf(stdout,"  -dxt5m : dxt5 style normal map\n");
    fprintf(stdout,"  -unsigned : convert normal maps to [0,1] for storing in unsigned formats (like DXT5)\n");

    

    fprintf(stdout,"\n");
    fprintf(stdout,"\n");
    fprintf(stdout,"Examples\n");
    fprintf(stdout,"  nvdxt -cubemap cubemap.dds -list cubemapfile.lst\n");
    fprintf(stdout,"  nvdxt -file test.tga -dxt1c\n");
    fprintf(stdout,"  nvdxt -file *.tga\n");
    fprintf(stdout,"  nvdxt -file c:\\temp\\*.tga\n");
    fprintf(stdout,"  nvdxt -file temp\\*.tga\n");
    fprintf(stdout,"  nvdxt -file height_field_in_alpha.tga -n3x3 -alpha -scale 10 -wrap\n");
    fprintf(stdout,"  nvdxt -file grey_scale_height_field.tga -n5x5 -rgb -scale 1.3\n");
    fprintf(stdout,"  nvdxt -file normal_map.tga -norm\n");
    fprintf(stdout,"  nvdxt -file image.tga -dudv -fade -fadeamount 10\n");
    fprintf(stdout,"  nvdxt -all -dxt3 -gamma -outdir .\\dds_dir -time\n");
    fprintf(stdout,"  nvdxt -file *.tga -depth -max -scale 0.5\n");


    fprintf(stdout,"Send comments, bug fixes and feature requests to doug@nvidia.com\n");

}





HRESULT callback(void * data, int miplevel, DWORD size)
{
    DWORD * ptr = (DWORD *)data;
    for(int i=0; i< size/4; i++)
    {
        DWORD c = *ptr++;
    }


    return 0;
}





HRESULT nvDXT::process_command_line(int argc, char * argv[])
{

    int i;
    i = 1;
    while(i<argc)
    {
        char * token = argv[i];


        bool bLegalCommand = false;
        
        if (strcmp(token, "-pause") == 0)
        {
            bPauseOnError = true;
            bLegalCommand = true;

        }
        
        
        if (strcmp(token, "-dxt5nm") == 0)
        {
            bDXT5NormalMap = true;
            bLegalCommand = true;

        }

        if (strcmp(token, "-alpha_modulate") == 0)
        {
            bAlphaModulate = true;
            bLegalCommand = true;

        }
        if (strcmp(token, "-binaryalpha") == 0)
        {
            bBinaryAlpha = true;
            bLegalCommand = true;

        }
        else if (strcmp(token, "-alpha_threshold") == 0)
        {

            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            bLegalCommand = true;

            BinaryAlphaThreshold = atoi(argv[i+1]);
            i++;
        }
        else if (strcmp(token, "-alphaborder") == 0)
        {
            bLegalCommand = true;
            bAlphaBorder = true;
        }
        else if (stricmp(token, "-alphaborderLeft") == 0)
        {
            bLegalCommand = true;
            bAlphaBorderLeft = true;
        }

        else if (stricmp(token, "-alphaborderRight") == 0)
        {
            bLegalCommand = true;
            bAlphaBorderRight = true;
        }
        else if (stricmp(token, "-alphaborderTop") == 0)
        {
            bLegalCommand = true;
            bAlphaBorderTop = true;
        }
        else if (stricmp(token, "-alphaborderBottom") == 0)
        {
            bLegalCommand = true;
            bAlphaBorderBottom = true;
        }


        else if (strcmp(token, "-flip") == 0)
        {
            bLegalCommand = true;
            bFlipTopToBottom = true;
        }
        else if (strcmp(token, "-deep") == 0)
        {

            bDeep = true;


            char * type = 0;

            if (i + 1 < argc)
                type = argv[i+1];

            if (type && type[0] != '-')
            {
                recursive_dirname = argv[i+1];
                i++;
            }
            else
            {
                char cwd[MAX_PATH_NAME];
                char * t;
                t = _getcwd(cwd, MAX_PATH_NAME);
                recursive_dirname = t;

            }

            bLegalCommand = true;


        }
        else if (strcmp(token, "-rescale") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }

            char * type = argv[i+1];

            if (strcmp(type, "nearest") == 0)
                rescaleImageType = RESCALE_NEAREST_POWER2;
            else if (strcmp(type, "hi") == 0)
                rescaleImageType = RESCALE_BIGGEST_POWER2;
            else if (strcmp(type, "lo") == 0)
                rescaleImageType = RESCALE_SMALLEST_POWER2;
            else if (strcmp(type, "next_lo") == 0)
                rescaleImageType = RESCALE_NEXT_SMALLEST_POWER2;
            else
            {
                error("Unknown option rescale'%s'\n", type);
                return ERROR_BAD_OPTION;
            }
            bLegalCommand = true;
            i++;
        }
        else if (strcmp(token, "-rel_scale") == 0)
        {

            if (i + 2 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }


            rescaleImageType = RESCALE_RELSCALE;

            scaleX = atof(argv[i+1]);
            scaleY = atof(argv[i+2]);
            bLegalCommand = true;
            i +=2;

        }
        else if (strcmp(token, "-clamp") == 0)
        {

            if (i + 2 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }

            ///rescaleImageType = RESCALE_CLAMP;

            bClamp = true;
            clampX = atof(argv[i+1]);
            clampY = atof(argv[i+2]);
            bLegalCommand = true;
            i +=2;

        }
        else if (strcmp(token, "-clampScale") == 0)
        {

            if (i + 2 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }

            ///rescaleImageType = RESCALE_CLAMP;

            bClampScale = true;
            clampScaleX = atof(argv[i+1]);
            clampScaleY = atof(argv[i+2]);
            bLegalCommand = true;
            i +=2;

        }
        else if (strcmp(token, "-window") == 0)
        {

            if (i + 4 >= argc)
            {
                error("incorrect number of arguments (window)\n");
                return ERROR_BAD_OPTION;

            }

            ///rescaleimagetype = rescale_clamp;

            bUseWindow = true;
            rect.left = atoi(argv[i+1]);
            rect.top = atoi(argv[i+2]);
            rect.right = atoi(argv[i+3]);
            rect.bottom = atoi(argv[i+4]);

            bLegalCommand = true;
            i +=4;

        }
        else if (strcmp(token, "-border") == 0)
        {
            bBorder = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-fadeamount") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }

            FadeAmount = atoi(argv[i+1]);
            bLegalCommand = true;
            i++;
        }
        else if (strcmp(token, "-nmips") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            SpecifiedMipMaps = atoi(argv[i+1]);
            bLegalCommand = true;
            i++;
        }
        else if (strcmp(token, "-force4") == 0)
        {
            bForceDXT1FourColors = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-bordercolor") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            BorderColor.u = strtoul(argv[i+1], 0, 16);
            bLegalCommand = true;
            i++;
        }
        else if (strcmp(token, "-rgbe") == 0)
        {
            bRGBE = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-fadealpha") == 0)
        {
            bFadeAlpha = true;
            bLegalCommand = true;
        }

        else if (strcmp(token, "-fadetoalpha") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            FadeToAlpha = atoi(argv[i+1]);
            bLegalCommand = true;
            i++;
        }

        else if (strcmp(token, "-fadecolor") == 0)
        {
            bFadeColor = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-fadetocolor") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            FadeToColor.u = strtoul(argv[i+1], 0, 16);
            bLegalCommand = true;
            i++;
        }

        else if (strcmp(token, "-nomipmap") == 0)
        {
            MipMapType = dNoMipMaps;
            bLegalCommand = true;
            //bGenMipMaps = false;
        }
        else if (strcmp(token, "-dxt1a") == 0)
        {
            TextureFormat =  kDXT1a;
            bLegalCommand = true;
        }
        else if ((strcmp(token, "-dxt1c") == 0) || (strcmp(token, "-dxt1") == 0))
        {
            TextureFormat =  kDXT1;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-dxt3") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kDXT3;
        }
        else if (strcmp(token, "-dxt5") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kDXT5;
        }
        else if (strcmp(token, "-cubemap") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            bCubeMap = true;
            cubeMapName = argv[i+1];
            bLegalCommand = true;
            i++;
        }
        else if (strcmp(token, "-volume") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            bVolumeTexture = true;
            volumeTextureName = argv[i+1];
            bLegalCommand = true;
            i++;
        }
        else if (strcmp(token, "-swap") == 0)
        {
            bSwapRGB = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-signed") == 0)
        {
            bSigned = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-quick") == 0)
        {
            bQuickCompress = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-profile") == 0)
        {
            HRESULT hr = SetProfileName(argv[i+1]);
            if (hr != 0)
            {
                error("Can't open profile %s\n",argv[i+1]);
                return (ERROR_CANT_OPEN_PROFILE);
            }

            bLegalCommand = true;
            i++;

        }
        else if (strcmp(token, "-all") == 0)
        {
            all_files = true;
            bLegalCommand = true;
        }


        else if (strcmp(token, "-dither") == 0)
        {
            bDitherColor = true;
            bLegalCommand = true;
        }

        else if (strcmp(token, "-sharpenMethod") == 0)
        {

            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            char * sharpenFilterName = argv[i+1];

            if (strcmp(sharpenFilterName, "None") == 0)
                SharpenFilterType = kSharpenFilterNone;
            else if (strcmp(sharpenFilterName, "Negative") == 0)
                SharpenFilterType = kSharpenFilterNegative;
            else if (strcmp(sharpenFilterName, "Lighter") == 0)
                SharpenFilterType = kSharpenFilterLighter;
            else if (strcmp(sharpenFilterName, "Darker") == 0)
                SharpenFilterType = kSharpenFilterDarker;
            else if (strcmp(sharpenFilterName, "ContrastMore") == 0)
                SharpenFilterType = kSharpenFilterContrastMore;
            else if (strcmp(sharpenFilterName, "ContrastLess") == 0)
                SharpenFilterType = kSharpenFilterContrastLess;
            else if (strcmp(sharpenFilterName, "Smoothen") == 0)
                SharpenFilterType = kSharpenFilterSmoothen;
            else if (strcmp(sharpenFilterName, "SharpenSoft") == 0)
                SharpenFilterType = kSharpenFilterSharpenSoft;
            else if (strcmp(sharpenFilterName, "SharpenMedium") == 0)
                SharpenFilterType = kSharpenFilterSharpenMedium;
            else if (strcmp(sharpenFilterName, "SharpenStrong") == 0)
                SharpenFilterType = kSharpenFilterSharpenStrong;
            else if (strcmp(sharpenFilterName, "FindEdges") == 0)
                SharpenFilterType = kSharpenFilterFindEdges;
            else if (strcmp(sharpenFilterName, "Contour") == 0)
                SharpenFilterType = kSharpenFilterContour;
            else if (strcmp(sharpenFilterName, "EdgeDetect") == 0)
                SharpenFilterType = kSharpenFilterEdgeDetect;
            else if (strcmp(sharpenFilterName, "EdgeDetectSoft") == 0)
                SharpenFilterType = kSharpenFilterEdgeDetectSoft;
            else if (strcmp(sharpenFilterName, "Emboss") == 0)
                SharpenFilterType = kSharpenFilterEmboss;
            else if (strcmp(sharpenFilterName, "MeanRemoval") == 0)
                SharpenFilterType = kSharpenFilterMeanRemoval;
            else if (strcmp(sharpenFilterName, "UnSharp") == 0)
            {
                SharpenFilterType = kSharpenFilterUnSharp;
                //
                if (i + 3 >= argc)
                {
                    error("incorrect number of arguments for kSharpenFilterUnSharp\n");
                return ERROR_BAD_OPTION;

                }
                unsharp_data.radius = atof(argv[i+1]);
                unsharp_data.amount = atof(argv[i+2]);
                unsharp_data.threshold = atoi(argv[i+3]);




                i += 3;


            }




            else if (strcmp(sharpenFilterName, "XSharpen") == 0)
            {
                SharpenFilterType = kSharpenFilterXSharpen;
                if (i + 2 >= argc)
                {
                    error("incorrect number of arguments for kSharpenFilterXSharpen\n");
                return ERROR_BAD_OPTION;

                
                }

                 xsharp_data.strength = atof(argv[i+1]);
                 xsharp_data.threshold = atof(argv[i+2]);

                //xsharpen_init(xsharpen_strength, xsharpen_threshold);

            }
            //else if (strcmp(sharpenFilterName, "FilterWarpSharp") == 0)
            //     SharpenFilterType = kFilterWarpSharp;
            else if (strcmp(sharpenFilterName, "Custom") == 0)
            {
                SharpenFilterType = kSharpenFilterCustom;
            }
            else
            {
                error("Unrecognized sharpen filter %s\n", sharpenFilterName);
                return ERROR_BAD_OPTION;
            }

            bLegalCommand = true;
            i++;
        }

        /*else if (strcmp(token, "-lambda") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        sharpenLambda = atof(argv[i+1]);
        i++;
        }
        else if (strcmp(token, "-sscale") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        sharpenScale = atof(argv[i+1]);
        i++;
        }
        else if (strcmp(token, "-mu") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        sharpenMu = atof(argv[i+1]);
        i++;
        }
        else if (strcmp(token, "-sb") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        sharpenBlur = atoi(argv[i+1]);
        i++;
        }
        else if (strcmp(token, "-sf2") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        sharpenFlavor2 = atof(argv[i+1]);
        i++;
        }       
        else if (strcmp(token, "-theta") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        sharpenTheta = atof(argv[i+1]);
        i++;
        }        
        else if (strcmp(token, "-edge_radius") == 0)
        {
        if (i + 1 >= argc)
        {
        printf("incorrect number of arguments\n");
        exit(1);
        }
        edgeRadius = atoi(argv[i+1]);
        i++;
        }
        else if (strcmp(token, "-tc") == 0)
        {
        bTwoComponents = true;
        }
        else if (strcmp(token, "-nms") == 0)
        {
        bNonMaximalSuppression = true;
        } */




        else if (strcmp(token, "-outdir") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                

            }
            output_dirname = argv[i+1];
            i++;
            bLegalCommand = true;

            bUserSpecifiedOutputDir = true;
        }
        else if (strcmp(token, "-outsamedir") == 0)
        {
            bOutputSameDir = true;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-overwrite") == 0)
        {
            bLegalCommand = true;
            bOverwrite = true;
        }
        else if (strcmp(token, "-forcewrite") == 0)
        {
            bLegalCommand = true;
            bForceWrite = true;
        }
        
        else if (strcmp(token, "-u4444") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  k4444;
        }
        else if (strcmp(token, "-u1555") == 0)
        {
            TextureFormat =  k1555;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-u565") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  k565;
        }
        else if (strcmp(token, "-u8888") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  k8888;
        }
        else if (strcmp(token, "-fp32x4") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kA32B32G32R32F;
        }
        else if (strcmp(token, "-fp32") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kR32F;
        }
        else if (strcmp(token, "-fp16x4") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kA16B16G16R16F;
        }
        else if (strcmp(token, "-u888") == 0)
        {
            TextureFormat =  k888;
            bLegalCommand = true;
        }
        else if (strcmp(token, "-u555") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  k555;
        }
        else if (strcmp(token, "-p8") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  k8;
        }
        else if (strcmp(token, "-p4") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  k4;
        }
        else if (strcmp(token, "-a8") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kA8;
        }
        else if (strcmp(token, "-A8L8") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kA8L8;
        }
        else if (strcmp(token, "-L8") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kL8;
        }
        else if (strcmp(token, "-timestamp") == 0)
        {
            bLegalCommand = true;
            timestamp = true;
        }
        else if (strcmp(token, "-list") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
            }
            list = true;

            listfile = argv[i+1];
            i++;
            bLegalCommand = true;
        }




        else if (stricmp(token, "-Point") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterPoint;
        }
        else if (stricmp(token, "-Box") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterBox;
        }
        else if (stricmp(token, "-Triangle") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterTriangle;
        }
        else if (stricmp(token, "-Quadratic") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterQuadratic;

        }
        else if (stricmp(token, "-Cubic") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterCubic;

        }
        else if (stricmp(token, "-Catrom") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterCatrom;

        }
        else if (stricmp(token, "-Mitchell") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterMitchell;

        }
        else if (stricmp(token, "-Gaussian") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterGaussian;

        }
        else if (stricmp(token, "-Sinc") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterSinc;

        }
        else if (stricmp(token, "-Bessel") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterBessel;

        }
        else if (stricmp(token, "-Hanning") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterHanning;

        }
        else if (stricmp(token, "-Hamming") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterHamming;

        }
        else if (stricmp(token, "-Blackman") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterBlackman;

        }
        else if (stricmp(token, "-Kaiser") == 0)
        {
            bLegalCommand = true;
            MIPFilterType = kMIPFilterKaiser;

        }
        /////////////////  prescale
        else if (stricmp(token, "-RescalePoint") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterPoint;
        }
        else if (stricmp(token, "-RescaleBox") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterBox;
        }
        else if (stricmp(token, "-RescaleTriangle") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterTriangle;
        }
        else if (stricmp(token, "-RescaleQuadratic") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterQuadratic;

        }
        else if (stricmp(token, "-RescaleCubic") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterCubic;

        }
        else if (stricmp(token, "-RescaleCatrom") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterCatrom;

        }
        else if (stricmp(token, "-RescaleMitchell") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterMitchell;

        }
        else if (stricmp(token, "-RescaleGaussian") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterGaussian;

        }
        else if (stricmp(token, "-RescaleSinc") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterSinc;

        }
        else if (stricmp(token, "-RescaleBessel") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterBessel;

        }
        else if (stricmp(token, "-RescaleHanning") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterHanning;

        }
        else if (stricmp(token, "-RescaleHamming") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterHamming;

        }
        else if (stricmp(token, "-RescaleBlackman") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterBlackman;

        }
        else if (stricmp(token, "-RescaleKaiser") == 0)
        {
            bLegalCommand = true;
            rescaleImageFilter = kMIPFilterKaiser;

        }
        else if (strcmp(token, "-n4") == 0)
        {
            bLegalCommand = true;
            normalMap.filterKernel = dFilter4x;
            normalMap.bEnableNormalMapConversion = true;
        }
        else if (strcmp(token, "-n3x3") == 0)
        {
            bLegalCommand = true;
            normalMap.filterKernel  = dFilter3x3;
            normalMap.bEnableNormalMapConversion = true;
        }
        else if (strcmp(token, "-n5x5") == 0)
        {
            bLegalCommand = true;
            normalMap.filterKernel  = dFilter5x5;                               
            normalMap.bEnableNormalMapConversion = true;
        }
        else if (strcmp(token, "-n7x7") == 0)
        {
            bLegalCommand = true;
            normalMap.filterKernel  = dFilter7x7;                               
            normalMap.bEnableNormalMapConversion = true;
        }
        else if (strcmp(token, "-n9x9") == 0)
        {
            bLegalCommand = true;
            normalMap.filterKernel  = dFilter9x9;                               
            normalMap.bEnableNormalMapConversion = true;
        }
        else if (strcmp(token, "-dudv") == 0)
        {
            bLegalCommand = true;

            normalMap.filterKernel  = dFilterDuDv;
            normalMap.bEnableNormalMapConversion = true;
        }
        else if (strcmp(token, "-unsigned") == 0)
        {
            bLegalCommand = true;
            normalMap.bSignedOutput = false;
        }
         
        else if (strcmp(token, "-v8u8") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kV8U8;
        }
        else if (strcmp(token, "-cxv8u8") == 0)
        {
            bLegalCommand = true;
            TextureFormat =  kCxV8U8;
        }
        else if (strcmp(token, "-depth") == 0)
        {
            //depthtype = 1;
            bLegalCommand = true;
            bDepthConversion = true;
        }
        else if (strcmp(token, "-to_height") == 0)
        {
            bLegalCommand = true;
            bToHeightMapConversion = true;
            toHeightScale = atof(argv[i+1]);
            toHeightBias = atof(argv[i+2]);
            i +=2;

        }
        else if (strcmp(token, "-alpha") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dALPHA;
        }
        else if (strcmp(token, "-rgb") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dAVERAGE_RGB;
        }
        else if (strcmp(token, "-biased") == 0)
        {
            bLegalCommand = true;

            normalMap.heightConversionMethod = dBIASED_RGB;
        }
        else if (strcmp(token, "-red") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dRED;
        }
        else if (strcmp(token, "-green") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dGREEN;
        }
        else if (strcmp(token, "-blue") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dBLUE;
        }
        else if (strcmp(token, "-max") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dMAX;
        }
        else if (strcmp(token, "-colorspace") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dCOLORSPACE;
        }
        else if (strcmp(token, "-norm") == 0)
        {
            bLegalCommand = true;
            normalMap.heightConversionMethod = dNORMALIZE;

        }
        else if (strcmp(token, "-diffusion") == 0)
        {
            bLegalCommand = true;
            bErrorDiffusion = true;

        }
        else if (strcmp(token, "-alphamod") == 0)
        {
            bLegalCommand = true;
            bAlphaModulate = true;

        }        
        else if (strcmp(token, "-aheight") == 0)
        {
            bLegalCommand = true;
            normalMap.alphaResult = dAlphaHeight;
        }
        else if (strcmp(token, "-aclear") == 0)
        {
            bLegalCommand = true;
            normalMap.alphaResult = dAlphaClear;
        }
        if (strcmp(token, "-awhite") == 0)
        {
            bLegalCommand = true;
            normalMap.alphaResult = dAlphaWhite;
        }

        else if (strcmp(token, "-file") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
            }
            specified_file = argv[i+1];

            if ( (specified_file.find("\\")  != -1) ||   (specified_file.find("/")  != -1) )
                bFullPathSpecified = true;
            bLegalCommand = true;

            i++;
        }
        else if (strcmp(token, "-output") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            output_specified_file = argv[i+1];

            bOutputFileSpecified = true;
            bLegalCommand = true;

            i++;
        }
        else if (strcmp(token, "-append") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            output_append = argv[i+1];

            bOutputFileAppend = true;
            bLegalCommand = true;

            i++;
        }

        else if (strcmp(token, "-24") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            char * mode = argv[i+1];
            i++;
            bLegalCommand = true;


            bUseMode24 = true;
            if (strcmp(mode, "dxt1c") == 0)
            {
                Mode24 = kDXT1;
            }
            else if (strcmp(mode, "dxt1") == 0)
            {
                Mode24 = kDXT1;
            }
            else if (strcmp(mode, "dxt1a") == 0)
            {
                Mode24 = kDXT1a;
            }
            else if (strcmp(mode, "dxt3") == 0)
            {
                Mode24 = kDXT3;
            }

            else if (strcmp(mode, "dxt5") == 0)
            {
                Mode24 = kDXT5;
            }

            else if (strcmp(mode, "u1555") == 0)
            {
                Mode24 = k1555;
            }

            else if (strcmp(mode, "u4444") == 0)
            {
                Mode24 = k4444;
            }

            else if (strcmp(mode, "u565") == 0)
            {
                Mode24 = k565;
            }

            else if (strcmp(mode, "u8888") == 0)
            {
                Mode24 = k8888;
            }
            else if (strcmp(mode, "u888") == 0)
            {
                Mode24 = k888;
            }
            else if (strcmp(mode, "u555") == 0)
            {
                Mode24 = k555;
            }
            else
            {
                printf("Unknown format %s, using DXT1\n", mode);
                Mode24 = kDXT1;
                bLegalCommand = false;
            }


        }
        else if (strcmp(token, "-32") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            bLegalCommand = true;


            char * mode = argv[i+1];
            i++;

            bUseMode32 = true;

            if (strcmp(mode, "dxt1c") == 0)
            {
                Mode32 = kDXT1;
            }
            if (strcmp(mode, "dxt1") == 0)
            {
                Mode32 = kDXT1;
            }
            else if (strcmp(mode, "dxt1a") == 0)
            {
                Mode32 = kDXT1a;
            }
            else if (strcmp(mode, "dxt3") == 0)
            {
                Mode32 = kDXT3;
            }

            else if (strcmp(mode, "dxt5") == 0)
            {
                Mode32 = kDXT5;
            }

            else if (strcmp(mode, "u1555") == 0)
            {
                Mode32 = k1555;
            }

            else if (strcmp(mode, "u4444") == 0)
            {
                Mode32 = k4444;
            }

            else if (strcmp(mode, "u565") == 0)
            {
                Mode32 = k565;
            }

            else if (strcmp(mode, "u8888") == 0)
            {
                Mode32 = k8888;
            }
            else if (strcmp(mode, "u888") == 0)
            {
                Mode32 = k888;
            }
            else if (strcmp(mode, "u555") == 0)
            {
                Mode32 = k555;
            }
            else
            {
                printf("Unknown format %s, using DXT1\n", mode);
                Mode32 = kDXT1;
                bLegalCommand = false;
            }

        }
        else if (strcmp(token, "-scale") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }

            normalMap.scale = atof(argv[i+1]);
            bLegalCommand = true;

            i++;

        }
        else if (strcmp(token, "-prescale") == 0)
        {
            if (i + 2 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;
                
            }
            rescaleImageType = RESCALE_PRESCALE;
            scaleX = atof(argv[i+1]);
            scaleY = atof(argv[i+2]);
            bLegalCommand = true;
            i+=2;

        }

        else if (strcmp(token, "-wrap") == 0)
        {
            normalMap.bWrap = true;
            bLegalCommand = true;

        }
        else if (strcmp(token, "-minz") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            normalMap.minz = atoi(argv[i+1]);
            bLegalCommand = true;
            i++;

        }
        else if (strcmp(token, "-gamma") == 0)
        {
            if (i + 1 >= argc)
            {
                error("incorrect number of arguments\n");
                return ERROR_BAD_OPTION;

            }
            FilterGamma = atof(argv[i+1]);
            bEnableFilterGamma = true;
            bLegalCommand = true;

            i++;
        }
        
        
        if (bLegalCommand == false)
        {
            fprintf(stdout, "Unknown option '%s'\n", token);
            return ERROR_BAD_ARG;

        }
        /*else
        {
            //  must be a file name
            specified_file = argv[i];
            if ( (specified_file.find("\\")  != -1) ||   (specified_file.find("/")  != -1) )
                bFullPathSpecified = true;
        } */
        i++;
    }
    return 0;
} 



// input_directory 
HRESULT  nvDXT::ProcessDirectory()
{

    HRESULT hr; 
    if (all_files)
    {
        hr = compress_all();
    }
    else if (specified_file.find("*") != -1)
    {
        hr = compress_some(const_cast<char *>(specified_file.c_str()));
    }

    else if (specified_file.size() > 0)
    {
        //inputfile = argv[optind];]


        hr = compress_image_file(const_cast<char *>(specified_file.c_str()), TextureFormat);
    }
    else
        fprintf(stdout, "nothing to do\n");

    return hr;

}


void WriteDTXnFile(DWORD count, void *buffer, void * userData)
{
    _write(fileout, buffer, count);

}


void ReadDTXnFile(DWORD count, void *buffer, void * userData)
{

    _read(filein, buffer, count);

}


typedef Vec4f float4;




#include <conio.h>



int main(int argc, char * argv[])
{
    /*
    float red, green, blue; 
    float red2, green2, blue2; 
    unsigned char rgbe[4];
    unsigned char rgbe2[4];


    rgbe[0] = 10;
    rgbe[1] = 20;
    rgbe[2] = 30;
    rgbe[3] = 128;

    rgbe2float(&red, &green, &blue, rgbe);   


    //float2rgbe(rgbe2, red, green, blue);

    float2rgbe(rgbe2, red, green, blue);


    rgbe2float(&red2, &green2, &blue2, rgbe2);   


    exit(0);*/


    fprintf(stdout,"%s\n", GetDXTCVersion());




    nvDXT dxt;

    /*int cw = _controlfp( 0, 0 );

    // Set the exception masks off, turn exceptions on

    //cw &=~(EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT | EM_ZERODIVIDE | EM_DENORMAL);
    //cw &=~(EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE | EM_DENORMAL);
    cw &=~(EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL);

    // Set the control word

    _controlfp( cw, MCW_EM );*/

#ifdef NVDXTDLL
    SetReadDTXnFile(ReadDTXnFile);
    SetWriteDTXnFile(WriteDTXnFile);
#endif


    dxt.specified_file = "";

    if (argc == 1)
    {
        usage();
        return ERROR_BAD_ARG;
    }

    dxt.output_dirname = ".";

    char cwd[MAX_PATH_NAME];
    char * startdir;
    startdir = _getcwd(cwd, MAX_PATH_NAME);
    dxt.input_dirname = startdir;

    dxt.MIPFilterType = kMIPFilterMitchell;    


    HRESULT hr = dxt.process_command_line(argc, argv);
    if (hr < 0)
    {

        return ERROR_BAD_ARG;
    }


    if (dxt.bUserSpecifiedOutputDir == true)
    {

        char cwd[MAX_PATH_NAME];
        char * t;
        t = _getcwd(cwd, MAX_PATH_NAME);


        int md = _chdir(dxt.output_dirname.c_str());

        _chdir(cwd);

        if (md != 0)
        {
            md = _mkdir(dxt.output_dirname.c_str());

            if (md == 0)
            {
                fprintf(stdout, "directory %s created\n", dxt.output_dirname.c_str());
                fflush(stdout);
            }
            else if (errno != EEXIST)
            {
                dxt.error("problem with output directory %s\n", dxt.output_dirname.c_str());
                return ERROR_BAD_OUTPUT_DIRECTORY;
            } 
            else
            {
                fprintf(stdout, "output directory %s\n", dxt.output_dirname.c_str());
                fflush(stdout);
            }
        }
    }


    hr = 0;



    if (dxt.list)
    {
        if (dxt.bVolumeTexture)
        {
            dxt.compress_volume_from_list();
        }
        else if (dxt.bCubeMap)
            hr = dxt.compress_cubemap_from_list();
        else
            hr = dxt.compress_list();
    }
    else if (dxt.bDeep)
    {

        dxt.compress_recursive(dxt.recursive_dirname.c_str(), hr);
        //compress_recursive("c:\\");
    }
    else
        hr = dxt.ProcessDirectory();



    fflush(stdout);
    fflush(stderr);
    return hr;
}











#include "sharpen.h"    

void unsharp_region
(RGBAImage &srcPR,
 RGBAImage &destPR,
 int width,
 int height,
 int bytes,
 double radius,
 double amount,
 int threshold,
 int x1,int x2,int y1,int y2);






/*void ReadDTXnFile (DWORD count, void *buffer)
{
// stubbed, we are not reading files
}*/






HRESULT nvDXT::ReadFileIntoList(const char * filename, StringArray & strArray)
{

    FILE *fp = fopen( listfile.c_str(), "r");

    if (fp == 0)
    {
        fprintf(stdout, "Can't open list file '%s'\n", listfile.c_str());
        return ERROR_CANT_OPEN_LIST_FILE;
    }


    bool inWhitespace = false;
    int c;
    std::string str;

    str.clear();

    while((c = fgetc(fp)) != EOF)
    {      

        switch(c)
        {
        case 13:
        case '\n':
            if (!inWhitespace)
            {
                strArray.push_back(str);
                str.clear();
            }

            inWhitespace = true;
            break;


        default:
            inWhitespace = false;
            str += c;

        }



    }

    if (!inWhitespace)
        strArray.push_back(str);

    fclose(fp);
    return 0;
}

HRESULT nvDXT::compress_cubemap_from_list()
{


    HRESULT hr = ReadFileIntoList(listfile.c_str(), cube_names);
    if (hr < 0)
        return hr;


    if (cube_names.size() != 6)
    {
        fprintf(stdout, "There are not six images in listfile '%s'\n", listfile.c_str());
        return ERROR_BAD_LIST_FILE_CONTENTS;
    }

    compress_cube();

    return 0;


}



/*
void ConvertNormalMapToHeight(float scale, float bias, int w, int h, void * vdata) 
{

DWORD *data = (DWORD *)vdata;
float *heightX = new float[w * h];
float *heightY = new float[w * h];

float *hptrX = heightX; 
float *hptrY = heightY; 
for ( int y = 0; y < h; y++ )
{
for ( int x = 0; x < w; x++ )
{
*hptrX++ = 0;
*hptrY++ = 0;
}
}



for ( int y = 0; y < h; y++ )
{
for ( int x = 0; x < w; x++ )
{
Vec3 n;
DWORD a;

ARGBToAlphaAndVector(data[y * w + x], a, n);
//n.normalize();


float pre;
if ( x == 0)
pre = 0;
else
pre = heightX[y * w + x - 1];

float * dst = &heightX[y * w + x];

if (n.z == 0)
{
int t = 1;
}
else 
{
float slope = n.x / n.z;

*dst = pre + slope;
}
}
}

for ( int x = 0; x < w; x++ )
{

for ( int y = 0; y < h; y++ )
{
Vec3 n;
DWORD a;

ARGBToAlphaAndVector(data[y * w + x], a, n);
//n.normalize();


float pre;
if ( y == 0)
pre = 0;
else
pre = heightY[(y-1) * w + x];

float * dst = &heightY[y * w + x];

if (n.z == 0)
{
int t = 1; // not chage
}
else 
{
float slope = n.y / n.z;

*dst = pre + slope;
}
}
}



hptrX = heightX; 
hptrY = heightY; 
for ( int y = 0; y < h; y++ )
{
for ( int x = 0; x < w; x++ )
{

*hptrX = (*hptrX + *hptrY) / 2.0;

*hptrX++;
*hptrY++;
}

}




float *hptr = heightX; 
float _min = *hptr;
float _max = *hptr;

for ( int y = 0; y < h; y++ )
{
for ( int x = 0; x < w; x++ )
{
float h = *hptr++;

if (h < _min)
_min = h; 
if (h > _max)
_max = h; 
}

}
float Zscale;

if (_max == _min)
Zscale = 1;
else
Zscale = 1./(_max - _min);



unsigned char *ptr = (unsigned char *)vdata;

for ( int y = 0; y < h; y++ )
{
for ( int x = 0; x < w; x++ )
{
float h = heightX[y * w + x];

// convert to [0-1]
h = (h - _min) * Zscale;

h = h * scale + bias;

// convert to color
h *= 255.0 + 0.5;

int grey = h;
if (grey > 255)
grey = 255;
if (grey < 0)
grey = 0;

*ptr++ = h;
*ptr++ = h;
*ptr++ = h;
*ptr++ = 0xFF;

}
}




delete [] heightX;
delete [] heightY;

}*/
//////////////////////////////////////////////////////



void nvDXT::error( LPSTR fmt, ... )
{ 
    va_list va;

    va_start( va, fmt );
    vprintf( fmt, va );
    va_end( va ); 

    fflush(stdout);

    if (bPauseOnError)
    {
        while(!kbhit())
            ;
    }
}