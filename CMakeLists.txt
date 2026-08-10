cmake_minimum_required(VERSION 3.24)
project(Devilz_Den LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE DEVILZ_DEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Keep the old executable harness in-tree for diagnostics, but it is not part
# of the injected runtime DLL.
list(FILTER DEVILZ_DEN_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(Devilz_Den SHARED ${DEVILZ_DEN_SOURCES})

set_target_properties(Devilz_Den PROPERTIES
    OUTPUT_NAME "Devilz_Den"
)

target_include_directories(Devilz_Den PRIVATE src)
target_compile_definitions(Devilz_Den PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if(WIN32)
    target_link_libraries(Devilz_Den PRIVATE psapi)

    add_executable(Devilz_Den_Smoke_Test tests/Runtime_Smoke_Test.cpp)
    target_compile_definitions(Devilz_Den_Smoke_Test PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

    add_executable(Devilz_Den_Address_Resolver_Test
        tests/GTA_Address_Resolver_Test.cpp
        src/Backend/Error/Error.cpp
        src/Backend/Process/Process_Memory_Reader.cpp
        src/Integrations/GTA5_Enhanced/Memory/GTA_Address_Resolver.cpp
    )
    target_include_directories(Devilz_Den_Address_Resolver_Test PRIVATE src)
    target_compile_definitions(Devilz_Den_Address_Resolver_Test PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    )

    add_executable(Devilz_Den_Target_Semantic_Validator_Test
        tests/GTA_Target_Semantic_Validator_Test.cpp
    )
    target_include_directories(Devilz_Den_Target_Semantic_Validator_Test PRIVATE src)
    target_compile_definitions(Devilz_Den_Target_Semantic_Validator_Test PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    )

    add_executable(Devilz_Den_Native_Manager_Test
        tests/GTA_Native_Manager_Test.cpp
        src/Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.cpp
    )
    target_include_directories(Devilz_Den_Native_Manager_Test PRIVATE src)
    target_compile_definitions(Devilz_Den_Native_Manager_Test PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    )

    if(BUILD_TESTING)
        add_test(NAME Devilz_Den.AddressResolver COMMAND Devilz_Den_Address_Resolver_Test)
        add_test(NAME Devilz_Den.TargetSemanticValidator COMMAND Devilz_Den_Target_Semantic_Validator_Test)
        add_test(NAME Devilz_Den.NativeManager COMMAND Devilz_Den_Native_Manager_Test)
    endif()

    set(DEVILZ_RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime/$<CONFIG>")
    add_custom_command(TARGET Devilz_Den POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVILZ_RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Devilz_Den>"
            "${DEVILZ_RUNTIME_DIR}/Devilz_Den.dll"
        COMMENT "Copying Devilz_Den.dll to injectable runtime directory"
        VERBATIM
    )

    if(MSVC)
        add_custom_command(TARGET Devilz_Den POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_PDB_FILE:Devilz_Den>"
                "${DEVILZ_RUNTIME_DIR}/Devilz_Den.pdb"
            VERBATIM
        )
    endif()
endif()

if(MSVC)
    target_compile_options(Devilz_Den PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Smoke_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Address_Resolver_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Target_Semantic_Validator_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Native_Manager_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()
