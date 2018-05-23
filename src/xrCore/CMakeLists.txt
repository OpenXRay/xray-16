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

#target_link_libraries(xrCore ${SDL2_LIBRARY} ${OPENSSL_LIBRARIES} ${LZO_LIBRARIES})
