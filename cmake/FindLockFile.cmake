
if(LOCKFILE_INCLUDE_DIR AND LOCKFILE_LIBRARIES)
   set(LOCKFILE_FOUND TRUE)

else(LOCKFILE_INCLUDE_DIR AND LOCKFILE_LIBRARIES)
  find_path(LOCKFILE_INCLUDE_DIR lockfile.h
      /usr/include
      /usr/local/include
      /opt/local/include
      $ENV{SystemDrive}/LockFile/include
      )

  find_library(LOCKFILE_LIBRARIES NAMES lockfile
      PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      $ENV{SystemDrive}/LockFile/lib
      )

  if(LOCKFILE_INCLUDE_DIR AND LOCKFILE_LIBRARIES)
    set(LOCKFILE_FOUND TRUE)
    message(STATUS "Found LockFile: ${LOCKFILE_INCLUDE_DIR}, ${LOCKFILE_LIBRARIES}")
  else(LOCKFILE_INCLUDE_DIR AND LOCKFILE_LIBRARIES)
    set(LOCKFILE_FOUND FALSE)
    message(FATAL_ERROR "LockFile not found.")
  endif(LOCKFILE_INCLUDE_DIR AND LOCKFILE_LIBRARIES)

  mark_as_advanced(LOCKFILE_INCLUDE_DIR LOCKFILE_LIBRARIES)

endif(LOCKFILE_INCLUDE_DIR AND LOCKFILE_LIBRARIES)
