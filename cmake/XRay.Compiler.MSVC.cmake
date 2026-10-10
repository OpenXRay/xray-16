include_guard()

include(GNUInstallDirs)

set(CMAKE_VS_USE_DEBUG_LIBRARIES "$<CONFIG:Debug>")

foreach(_xray_flags_var CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MIXED CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_MIXED)
    string(REGEX REPLACE "/Z(7|i|I)" "" ${_xray_flags_var} "${${_xray_flags_var}}")
endforeach()
string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_compile_options(
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CONFIG:ReleaseMasterGold>>>:/EHsc>
    $<$<COMPILE_LANGUAGE:C,CXX>:/Z7>
    $<$<COMPILE_LANGUAGE:C,CXX>:/Zc:inline>
    $<$<COMPILE_LANGUAGE:C,CXX>:/bigobj>
    $<$<COMPILE_LANGUAGE:C,CXX>:/arch:AVX2>
    $<$<COMPILE_LANGUAGE:C,CXX>:/wd4201>
    $<$<COMPILE_LANGUAGE:C,CXX>:/wd4251>
    $<$<COMPILE_LANGUAGE:C,CXX>:/wd4275>
    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,ReleaseMasterGold>>:/Ot>
    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,ReleaseMasterGold>>:/GS->
    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,ReleaseMasterGold>>:/fp:fast>
    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,ReleaseMasterGold>>:/Qfast_transcendentals>
    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,ReleaseMasterGold>>:/Gy>
)

add_compile_definitions($<$<CONFIG:ReleaseMasterGold>:_HAS_EXCEPTIONS=0>)

add_link_options(
    $<$<CONFIG:Release,ReleaseMasterGold>:/OPT:REF>
    $<$<CONFIG:Release,ReleaseMasterGold>:/OPT:ICF>
)

set(XRAY_DISABLE_WARNINGS "/w")
