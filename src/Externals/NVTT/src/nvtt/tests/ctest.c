
#include <stdio.h>

#include <nvtt/nvtt_wrapper.h>


int main(void)
{
	NvttInputOptions inputOptions = 0;
	NvttOutputOptions outputOptions = 0;
	NvttCompressionOptions compressionOptions = 0;

	const unsigned int img[16*16];
	
	memset(img, 0, sizeof(unsigned int) * 16 * 16);

	inputOptions = nvttCreateInputOptions();
	nvttSetInputOptionsTextureLayout(inputOptions, NVTT_TextureType_2D, 16, 16, 1);
	nvttSetInputOptionsMipmapData(inputOptions, img, 16, 16, 1, 0, 0);

	outputOptions = nvttCreateOutputOptions();
	nvttSetOutputOptionsFileName(outputOptions, "output.dds");

	compressionOptions = nvttCreateCompressionOptions();
	nvttSetCompressionOptionsFormat(compressionOptions, NVTT_Format_BC1);

	nvttCompress(inputOptions, outputOptions, compressionOptions);

	nvttDestroyCompressionOptions(compressionOptions);
	nvttDestroyOutputOptions(outputOptions);
	nvttDestroyInputOptions(inputOptions);

	return 0;
}

