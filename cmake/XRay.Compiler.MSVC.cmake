include_guard()

# The MSVC compiler settings:
# Set properties:
set(CMAKE_VS_USE_DEBUG_LIBRARIES "$<CONFIG:Debug>")

# Clear predefined flags which we going to define ourselves
string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # exceptions
string(REGEX REPLACE "/Z(7|i|I)" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}) # debug information format

# Enable standard C++ exceptions everywhere except ReleaseMasterGold
add_compile_options($<$<NOT:$<CONFIG:ReleaseMasterGold>>:/EHsc>)

# Disable MS STL exceptions on ReleaseMasterGold
add_compile_definitions($<$<CONFIG:ReleaseMasterGold>:_HAS_EXCEPTIONS=0>)

add_compile_options($<IF:$<CONFIG:ReleaseMasterGold>,/Z7,/Zi>)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    add_compile_options(/MP)
endif()

add_compile_options($<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:/arch:SSE2>)
add_compile_options($<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:/arch:AVX2>)

# Disable specific warnings
add_compile_options(
    /wd4201 # nonstandard extension used : nameless struct/union
    /wd4251 # class 'x' needs to have dll-interface to be used by clients of class 'y'
    /wd4275 # non dll-interface class 'x' used as base for dll-interface class 'y'
)

# The MSVC linker settings:
add_link_options("/LARGEADDRESSAWARE")

set(XRAY_DISABLE_WARNINGS "/w")

add_compile_options(
    $<$<CONFIG:Release,ReleaseMasterGold>:/Ot>
    $<$<CONFIG:Release,ReleaseMasterGold>:/GS->
    $<$<CONFIG:Release,ReleaseMasterGold>:/fp:fast>
    $<$<CONFIG:Release,ReleaseMasterGold>:/Qfast_transcendentals>
    $<$<CONFIG:Release,ReleaseMasterGold>:/Gy>
)

add_link_options(
    $<$<CONFIG:Release,ReleaseMasterGold>:/OPT:REF>
    $<$<CONFIG:Release,ReleaseMasterGold>:/OPT:ICF>
)
