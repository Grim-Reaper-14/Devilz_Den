cmake_minimum_required(VERSION 3.24)

foreach(required_var
    DEVILZ_DECOMPILE_SCRIPTS_REPOSITORY
    DEVILZ_DECOMPILE_SCRIPTS_REVISION
    DEVILZ_DECOMPILE_SCRIPTS_DESTINATION)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} must be provided")
    endif()
endforeach()

find_program(DEVILZ_GIT_EXECUTABLE NAMES git REQUIRED)

set(repo "${DEVILZ_DECOMPILE_SCRIPTS_REPOSITORY}")
set(revision "${DEVILZ_DECOMPILE_SCRIPTS_REVISION}")
set(destination "${DEVILZ_DECOMPILE_SCRIPTS_DESTINATION}")

if(EXISTS "${destination}" AND NOT IS_DIRECTORY "${destination}")
    message(FATAL_ERROR "Decompile-script destination exists but is not a directory: ${destination}")
endif()

if(EXISTS "${destination}/.git")
    execute_process(
        COMMAND "${DEVILZ_GIT_EXECUTABLE}" -C "${destination}" remote get-url origin
        RESULT_VARIABLE origin_result
        OUTPUT_VARIABLE origin_url
        ERROR_VARIABLE origin_error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT origin_result EQUAL 0)
        message(FATAL_ERROR "Unable to inspect decompile-script checkout origin: ${origin_error}")
    endif()
    if(NOT origin_url STREQUAL repo)
        message(FATAL_ERROR
            "Refusing to modify ${destination}: origin is '${origin_url}', expected '${repo}'")
    endif()
else()
    if(EXISTS "${destination}")
        file(GLOB existing_entries LIST_DIRECTORIES true "${destination}/*")
        if(existing_entries)
            message(FATAL_ERROR
                "Refusing to initialize non-empty decompile-script directory: ${destination}")
        endif()
    else()
        get_filename_component(destination_parent "${destination}" DIRECTORY)
        file(MAKE_DIRECTORY "${destination_parent}")
    endif()

    execute_process(
        COMMAND "${DEVILZ_GIT_EXECUTABLE}" init "${destination}"
        RESULT_VARIABLE init_result
        OUTPUT_VARIABLE init_output
        ERROR_VARIABLE init_error)
    if(NOT init_result EQUAL 0)
        message(FATAL_ERROR "Unable to initialize decompile-script checkout: ${init_error}")
    endif()

    execute_process(
        COMMAND "${DEVILZ_GIT_EXECUTABLE}" -C "${destination}" remote add origin "${repo}"
        RESULT_VARIABLE remote_result
        OUTPUT_VARIABLE remote_output
        ERROR_VARIABLE remote_error)
    if(NOT remote_result EQUAL 0)
        message(FATAL_ERROR "Unable to configure decompile-script origin: ${remote_error}")
    endif()
endif()

execute_process(
    COMMAND "${DEVILZ_GIT_EXECUTABLE}" -C "${destination}" fetch --depth 1 origin "${revision}"
    RESULT_VARIABLE fetch_result
    OUTPUT_VARIABLE fetch_output
    ERROR_VARIABLE fetch_error)
if(NOT fetch_result EQUAL 0)
    message(FATAL_ERROR "Unable to fetch decompile-script revision ${revision}: ${fetch_error}")
endif()

execute_process(
    COMMAND "${DEVILZ_GIT_EXECUTABLE}" -C "${destination}" checkout --detach --force FETCH_HEAD
    RESULT_VARIABLE checkout_result
    OUTPUT_VARIABLE checkout_output
    ERROR_VARIABLE checkout_error)
if(NOT checkout_result EQUAL 0)
    message(FATAL_ERROR "Unable to check out decompile-script revision ${revision}: ${checkout_error}")
endif()

execute_process(
    COMMAND "${DEVILZ_GIT_EXECUTABLE}" -C "${destination}" rev-parse HEAD
    RESULT_VARIABLE head_result
    OUTPUT_VARIABLE head_revision
    ERROR_VARIABLE head_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT head_result EQUAL 0)
    message(FATAL_ERROR "Unable to verify decompile-script revision: ${head_error}")
endif()
if(NOT head_revision STREQUAL revision)
    message(FATAL_ERROR
        "Decompile-script checkout verification failed: expected ${revision}, got ${head_revision}")
endif()

message(STATUS "GTA V Enhanced decompile scripts ready at ${destination} (${head_revision})")
