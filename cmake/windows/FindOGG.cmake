find_path(OGG_INCLUDE_DIR
    NAMES ogg.h
    PATHS "${CMAKE_SOURCE_DIR}/sdk/include/ogg"
    NO_DEFAULT_PATH
)

find_library(OGG_LIBRARY
    NAMES libogg_static
    PATHS "${CMAKE_SOURCE_DIR}/sdk/libraries/${ARCH_TYPE}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(
    OGG_INCLUDE_DIR
    OGG_LIBRARY
)

add_library(Ogg_Ogg STATIC IMPORTED GLOBAL)
add_library(Ogg::Ogg ALIAS Ogg_Ogg)

set_target_properties(Ogg_Ogg PROPERTIES
    IMPORTED_LOCATION "${OGG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIR}"
)
