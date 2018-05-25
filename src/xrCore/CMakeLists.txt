cmake_minimum_required(VERSION 3.5)

add_dir (
	"."
	"Animation"
	"Compression"
	"Containers"
	"Crypto"
	"Math"
	"Media"
	"PostProcess"
	"Text"
	"Threading"
	"XML"
        )

message( "path "  ${CMAKE_CURRENT_SOURCE_DIR} ) 

add_library(xrCore SHARED ${XRCORE__SOURCES} ${XRCORE__INCLUDES})

find_library(SDL2_LIB SDL2)
find_library(LZO_LIBRARY lzo2)
find_library(CRYPTO_LIBRARY crypto++)
find_library(PUGIXML_LIBRARY pugixml)
target_link_libraries(xrCore ${SDL2_LIB} ${OPENSSL_LIBRARIES} ${LZO_LIBRARY} ${CRYPTO_LIBRARY} ${PUGIXML_LIBRARY})
