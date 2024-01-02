find_path(THEORA_INCLUDE_DIR
    NAMES theora/theora.h
    PATHS "${CMAKE_SOURCE_DIR}/sdk/include"
    NO_DEFAULT_PATH
)

find_library(THEORA_LIBRARY
    NAMES libtheora_static
    PATHS "${CMAKE_SOURCE_DIR}/sdk/libraries/${ARCH_TYPE}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

add_library(THEORA_THEORA STATIC IMPORTED GLOBAL)
add_library(Theora::Theora ALIAS THEORA_THEORA)

set_target_properties(THEORA_THEORA PROPERTIES
    IMPORTED_LOCATION "${THEORA_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${THEORA_INCLUDE_DIR}"
)

mark_as_advanced(
    THEORA_INCLUDE_DIR
    THEORA_LIBRARY
)
