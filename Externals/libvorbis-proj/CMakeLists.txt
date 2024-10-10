add_library(libvorbis STATIC)

add_library(Vorbis::Vorbis ALIAS libvorbis)

# TODO disable unity build?

target_include_directories(libvorbis
    PUBLIC
    ../libvorbis/include
    ../libogg/include
)

target_compile_definitions(libvorbis
    PRIVATE
    _USRDLL
    LIBVORBIS_EXPORTS
)

target_link_options(libvorbis PRIVATE 
    /DEF:../libvorbis/win32/vorbis.def
)

target_sources(libvorbis
    PRIVATE
    ../libvorbis/include/vorbis/codec.h
    ../libvorbis/include/vorbis/vorbisenc.h
    ../libvorbis/include/vorbis/vorbisfile.h
    ../libvorbis/lib/analysis.c
    ../libvorbis/lib/backends.h
    ../libvorbis/lib/bitrate.c
    ../libvorbis/lib/bitrate.h
    ../libvorbis/lib/block.c
    ../libvorbis/lib/books/coupled/res_books_stereo.h
    ../libvorbis/lib/books/floor/floor_books.h
    ../libvorbis/lib/books/uncoupled/res_books_uncoupled.h
    ../libvorbis/lib/codebook.c
    ../libvorbis/lib/codebook.h
    ../libvorbis/lib/codec_internal.h
    ../libvorbis/lib/envelope.c
    ../libvorbis/lib/envelope.h
    ../libvorbis/lib/floor0.c
    ../libvorbis/lib/floor1.c
    ../libvorbis/lib/highlevel.h
    ../libvorbis/lib/info.c
    ../libvorbis/lib/lookup.c
    ../libvorbis/lib/lookup.h
    ../libvorbis/lib/lookup_data.h
    ../libvorbis/lib/lpc.c
    ../libvorbis/lib/lpc.h
    ../libvorbis/lib/lsp.c
    ../libvorbis/lib/lsp.h
    ../libvorbis/lib/mapping0.c
    ../libvorbis/lib/masking.h
    ../libvorbis/lib/mdct.c
    ../libvorbis/lib/mdct.h
    ../libvorbis/lib/misc.h
    ../libvorbis/lib/modes/floor_all.h
    ../libvorbis/lib/modes/psych_11.h
    ../libvorbis/lib/modes/psych_16.h
    ../libvorbis/lib/modes/psych_44.h
    ../libvorbis/lib/modes/psych_8.h
    ../libvorbis/lib/modes/residue_16.h
    ../libvorbis/lib/modes/residue_44.h
    ../libvorbis/lib/modes/residue_44u.h
    ../libvorbis/lib/modes/residue_8.h
    ../libvorbis/lib/modes/setup_11.h
    ../libvorbis/lib/modes/setup_16.h
    ../libvorbis/lib/modes/setup_22.h
    ../libvorbis/lib/modes/setup_32.h
    ../libvorbis/lib/modes/setup_44.h
    ../libvorbis/lib/modes/setup_44u.h
    ../libvorbis/lib/modes/setup_8.h
    ../libvorbis/lib/modes/setup_X.h
    ../libvorbis/lib/os.h
    ../libvorbis/lib/psy.c
    ../libvorbis/lib/psy.h
    ../libvorbis/lib/registry.c
    ../libvorbis/lib/registry.h
    ../libvorbis/lib/res0.c
    ../libvorbis/lib/scales.h
    ../libvorbis/lib/sharedbook.c
    ../libvorbis/lib/smallft.c
    ../libvorbis/lib/smallft.h
    ../libvorbis/lib/synthesis.c
    ../libvorbis/lib/vorbisenc.c
    ../libvorbis/lib/window.c
    ../libvorbis/lib/window.h
)

add_library(libvorbisfile STATIC)

add_library(Vorbis::VorbisFile ALIAS libvorbisfile)

target_include_directories(libvorbisfile
    PRIVATE
    ../libvorbis/include
    ../libogg/include
)

target_compile_definitions(libvorbisfile
    PRIVATE
    _USRDLL
    LIBVORBISFILE_EXPORTS
)

target_sources(libvorbisfile
    PRIVATE
    ../libvorbis/lib/vorbisfile.c
    ../libvorbis/include/vorbis/vorbisfile.h
)
