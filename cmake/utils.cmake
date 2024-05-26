function(target_sources_grouped)
    cmake_parse_arguments(
        PARSED_ARGS
        ""
        "TARGET;NAME;SCOPE"
        "FILES"
        ${ARGN}
    )

    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target name")
    endif()

    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "You must provide a source group name")
    endif()

    if(NOT PARSED_ARGS_SCOPE)
        set(PARSED_ARGS_SCOPE PRIVATE)
    endif()

    target_sources(${PARSED_ARGS_TARGET} ${PARSED_ARGS_SCOPE} ${PARSED_ARGS_FILES})

    source_group(${PARSED_ARGS_NAME} FILES ${PARSED_ARGS_FILES})
endfunction()

macro(set_git_info)
    execute_process(COMMAND git rev-parse --verify HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_SHA1
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "git commit: ${GIT_SHA1}")

    execute_process(COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_BRANCH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "git branch: ${GIT_BRANCH}")
endmacro()

function(calculate_xray_build_id output)
    set(XRAY_START_DAY   31)
    set(XRAY_START_MONTH 1)
    set(XRAY_START_YEAR  1999)

    set(DAYS_IN_MONTH 0 31 28 31 30 31 30 31 31 30 31 30 31) # first is dummy

    # Acquire timestamp in "date month year" format
    string(TIMESTAMP current_date "%d %m %Y")

    # Transform string into a list, then extract 3 separate variables
    string(REPLACE " " ";" current_date_list ${current_date})
    list(GET current_date_list 0 CURRENT_DATE_DAY)
    list(GET current_date_list 1 CURRENT_DATE_MONTH)
    list(GET current_date_list 2 CURRENT_DATE_YEAR)

    # Check if current date is before the start date
    # See https://github.com/OpenXRay/xray-16/issues/1611
    if ( (CURRENT_DATE_YEAR LESS XRAY_START_YEAR)
        OR ( (CURRENT_DATE_YEAR EQUAL XRAY_START_YEAR)
            AND (CURRENT_DATE_MONTH LESS XRAY_START_MONTH)
            OR ( (CURRENT_DATE_MONTH EQUAL XRAY_START_MONTH)
                AND (CURRENT_DATE_DAY LESS XRAY_START_DAY) ) ) )
        set(${output} 0 PARENT_SCOPE)
        return()
    endif()

    # Calculate XRAY build ID
    math(EXPR build_id "(${CURRENT_DATE_YEAR} - ${XRAY_START_YEAR}) * 365 + ${CURRENT_DATE_DAY} - ${XRAY_START_DAY}")

    set(it 1)
    while(it LESS CURRENT_DATE_MONTH)
        list(GET DAYS_IN_MONTH ${it} days)
        math(EXPR build_id "${build_id} + ${days}")

        math(EXPR it "${it} + 1")
    endwhile()

    set(it 1)
    while(it LESS XRAY_START_MONTH)
        list(GET DAYS_IN_MONTH ${it} days)
        math(EXPR build_id "${build_id} - ${days}")

        math(EXPR it "${it} + 1")
    endwhile()

    # Set requested variable
    set(${output} ${build_id} PARENT_SCOPE)
endfunction()
