#https://raw.githubusercontent.com/Andrettin/Wyrmgus/master/cmake/modules/FindTheora.cmake
# Option for build or not Theora

# Copyright (c) 2006,2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if(THEORA_INCLUDE_DIR AND THEORA_LIB_LIBRARIES AND THEORA_VORBIS_LIBRARIES AND THEORA_OGG_LIBRARIES)
	# Already in cache, be silent
	set(Theora_FIND_QUIETLY TRUE)	
endif(THEORA_INCLUDE_DIR AND THEORA_LIB_LIBRARIES AND THEORA_VORBIS_LIBRARIES AND THEORA_OGG_LIBRARIES)

FIND_PATH(THEORA_INCLUDE_DIR theora/theora.h)

FIND_LIBRARY(THEORA_OGG_LIBRARIES NAMES libogg libogg_static ogg ogg_static)

FIND_LIBRARY(THEORA_VORBIS_LIBRARIES NAMES libvorbis libvorbis_static vorbis vorbis_static)

FIND_LIBRARY(THEORA_LIB_LIBRARIES NAMES libtheora libtheora_static theora theora_static)

if(THEORA_LIB_LIBRARIES AND THEORA_VORBIS_LIBRARIES AND THEORA_OGG_LIBRARIES AND THEORA_INCLUDE_DIR)
	set(THEORA_LIBRARY ${THEORA_LIB_LIBRARIES} ${THEORA_OGG_LIBRARIES} ${THEORA_VORBIS_LIBRARIES})
	set(THEORA_FOUND TRUE)
endif(THEORA_LIB_LIBRARIES AND THEORA_VORBIS_LIBRARIES AND THEORA_OGG_LIBRARIES AND THEORA_INCLUDE_DIR)

if (THEORA_FOUND)
  if (NOT Theora_FIND_QUIETLY)
     MESSAGE( STATUS "theora found: includes in ${THEORA_INCLUDE_DIR}, library in ${THEORA_LIBRARY}")
  endif (NOT Theora_FIND_QUIETLY)
else (THEORA_FOUND)
  if (Theora_FIND_REQUIRED)
     MESSAGE( FATAL_ERROR "theora not found")
  endif (Theora_FIND_REQUIRED)
endif (THEORA_FOUND)


MARK_AS_ADVANCED(THEORA_INCLUDE_DIR THEORA_LIBRARY)
