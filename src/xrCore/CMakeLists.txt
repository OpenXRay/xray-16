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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include)

add_library(xrCore SHARED ${XRCORE__SOURCES} ${XRCORE__INCLUDES})

set_target_properties(xrCore PROPERTIES PREFIX "")
target_link_libraries(xrCore ${SDL2_LIB} ${OPENSSL_LIBRARIES} ${LZO_LIBRARY} ${CRYPTO_LIBRARY} ${PUGIXML_LIBRARY})
