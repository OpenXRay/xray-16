<<<<<<< HEAD
project(xrCore)

list(APPEND DIRS
=======
add_dir (
>>>>>>> upstream/xd_dev
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

<<<<<<< HEAD
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/pugixml/src ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} ${SDL2_LIB} ${OPENSSL_LIBRARIES} ${LZO_LIBRARY} ${CRYPTO_LIBRARY} ${PUGIXML_LIBRARY})
=======
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include)

add_library(xrCore SHARED ${XRCORE__SOURCES} ${XRCORE__INCLUDES})

set_target_properties(xrCore PROPERTIES PREFIX "")
target_link_libraries(xrCore ${SDL2_LIB} ${OPENSSL_LIBRARIES} ${LZO_LIBRARY} ${CRYPTO_LIBRARY} ${PUGIXML_LIBRARY})
>>>>>>> upstream/xd_dev
