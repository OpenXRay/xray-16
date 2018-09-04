#https://raw.githubusercontent.com/harningt/cryptoface/7332799df7d9c78012eaa153f6598dcfa7debbc4/Modules/FindCrypto%2B%2B.cmake
# - Find Crypto++

if(CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)
   set(CRYPTO++_FOUND TRUE)

else(CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)
  find_path(CRYPTO++_INCLUDE_DIR cryptlib.h
      /usr/include/crypto++
      /usr/include/cryptopp
      /usr/local/include/crypto++
      /usr/local/include/cryptopp
      /opt/local/include/crypto++
      /opt/local/include/cryptopp
      $ENV{SystemDrive}/Crypto++/include
      )

  find_library(CRYPTO++_LIBRARIES NAMES cryptopp
      PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      $ENV{SystemDrive}/Crypto++/lib
      )

  if(CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)
    set(CRYPTO++_FOUND TRUE)
    message(STATUS "Found Crypto++: ${CRYPTO++_INCLUDE_DIR}, ${CRYPTO++_LIBRARIES}")
  else(CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)
    set(CRYPTO++_FOUND FALSE)
    message(FATAL_ERROR "Crypto++ not found.")
  endif(CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)

  mark_as_advanced(CRYPTO++_INCLUDE_DIR CRYPTO++_LIBRARIES)

endif(CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)
