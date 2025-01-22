add_library(libtheora STATIC)

add_library(Theora::Theora ALIAS libtheora)

# TODO disable unity build?

target_include_directories(libtheora
    PUBLIC
    ../libtheora/include
    PRIVATE
    ../libtheora/lib
    ../libogg/include
)

target_compile_definitions(libtheora
    PRIVATE
    _CRT_NONSTDC_NO_DEPRECATE
    _BIND_TO_CURRENT_CRT_VERSION
    _USRDLL
    LIBTHEORA_EXPORTS
    #OC_X86_ASM
)

target_sources(libtheora
    PRIVATE
    ../libtheora/include/theora/codec.h
    ../libtheora/include/theora/theora.h
    ../libtheora/include/theora/theoradec.h
    ../libtheora/lib/analyze.c
    ../libtheora/lib/apiwrapper.c
    ../libtheora/lib/apiwrapper.h
    ../libtheora/lib/bitpack.c
    ../libtheora/lib/bitpack.h
    ../libtheora/lib/dct.h
    ../libtheora/lib/decapiwrapper.c
    ../libtheora/lib/decinfo.c
    ../libtheora/lib/decint.h
    ../libtheora/lib/decode.c
    ../libtheora/lib/dequant.c
    ../libtheora/lib/dequant.h
    ../libtheora/lib/encapiwrapper.c
    ../libtheora/lib/encfrag.c
    ../libtheora/lib/encinfo.c
    ../libtheora/lib/encint.h
    ../libtheora/lib/encode.c
    #../libtheora/lib/encoder_disabled.c
    ../libtheora/lib/enquant.c
    ../libtheora/lib/enquant.h
    ../libtheora/lib/fdct.c
    ../libtheora/lib/fragment.c
    ../libtheora/lib/huffdec.c
    ../libtheora/lib/huffdec.h
    ../libtheora/lib/huffenc.c
    ../libtheora/lib/huffenc.h
    ../libtheora/lib/huffman.h
    ../libtheora/lib/idct.c
    ../libtheora/lib/info.c
    ../libtheora/lib/internal.c
    ../libtheora/lib/internal.h
    ../libtheora/lib/mathops.c
    ../libtheora/lib/mathops.h
    ../libtheora/lib/mcenc.c
    ../libtheora/lib/modedec.h
    ../libtheora/lib/ocintrin.h
    ../libtheora/lib/quant.c
    ../libtheora/lib/quant.h
    ../libtheora/lib/rate.c
    ../libtheora/lib/state.c
    ../libtheora/lib/tokenize.c
    ../libtheora/lib/x86_vc/mmxencfrag.c
    ../libtheora/lib/x86_vc/mmxfdct.c
    ../libtheora/lib/x86_vc/mmxfrag.c
    ../libtheora/lib/x86_vc/mmxidct.c
    ../libtheora/lib/x86_vc/mmxstate.c
    ../libtheora/lib/x86_vc/x86cpu.c
    ../libtheora/lib/x86_vc/x86cpu.h
    ../libtheora/lib/x86_vc/x86enc.c
    ../libtheora/lib/x86_vc/x86state.c
)
