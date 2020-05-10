macro(add_dir DIRS)
  foreach(dir ${DIRS})
    message( "adding  ${dir} to ${PROJECT_NAME}")
    include_directories  (${dir} )
    file( GLOB ${dir}__INCLUDES_H ${dir} ${dir}/*.h)
    file( GLOB ${dir}__INCLUDES_HPP ${dir} ${dir}/*.hpp)
    list( APPEND ${PROJECT_NAME}__INCLUDES ${${dir}__INCLUDES_H} ${${dir}__INCLUDES_HPP} )
    file( GLOB ${dir}__SOURCES_CPP ${dir} ${dir}/*.cpp ${dir}/*.cxx)
    file( GLOB ${dir}__SOURCES_C ${dir} ${dir}/*.c)
    list( APPEND ${PROJECT_NAME}__SOURCES ${${dir}__SOURCES_C} ${${dir}__SOURCES_CPP} )
  endforeach()
endmacro()


# ------------------------------------------
# Detect arch type ( x86 or x64 )
# ------------------------------------------
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH_TYPE x64)
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(ARCH_TYPE x86)
endif()