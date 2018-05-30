project(xrCore)

list(APPEND DIRS
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

add_dir("${DIRS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/pugixml/src ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} ${SDL2_LIB} ${OPENSSL_LIBRARIES} ${LZO_LIBRARY} ${CRYPTO_LIBRARY} ${PUGIXML_LIBRARY})
