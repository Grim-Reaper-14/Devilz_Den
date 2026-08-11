cmake_minimum_required(VERSION 3.24)
project(Devilz_Den LANGUAGES CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE DEVILZ_DEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Domain modules added during the menu refactor are also listed explicitly.
# This avoids an MSBuild edge case where CONFIGURE_DEPENDS discovers a new
# source during the build but the already-loaded .vcxproj reaches link before
# that source is compiled. list(REMOVE_DUPLICATES) keeps clean fresh builds
# from compiling these files twice when the glob already contains them.
list(APPEND DEVILZ_DEN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Menu/Lua/Lua_Page.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Menu/Network/Network_Page.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Menu/Themes/Menu_Theme.cpp"
)
list(REMOVE_DUPLICATES DEVILZ_DEN_SOURCES)

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
    FetchContent_Declare(
        dear_imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.9
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(dear_imgui)
    if(NOT dear_imgui_POPULATED)
        FetchContent_Populate(dear_imgui)
    endif()

    add_library(Devilz_Den_ImGui STATIC
        "${dear_imgui_SOURCE_DIR}/imgui.cpp"
        "${dear_imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${dear_imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${dear_imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${dear_imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp"
        "${dear_imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    )
    target_include_directories(Devilz_Den_ImGui PUBLIC
        "${dear_imgui_SOURCE_DIR}"
        "${dear_imgui_SOURCE_DIR}/backends"
    )
    target_compile_definitions(Devilz_Den_ImGui PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(Devilz_Den_ImGui PUBLIC d3d12 d3dcompiler dxgi)

    target_link_libraries(Devilz_Den PRIVATE psapi Devilz_Den_ImGui)

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
        src/Integrations/GTA5_Enhanced/Runtime/GTA_Build_Registry.cpp
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
    target_compile_options(Devilz_Den_ImGui PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Smoke_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Address_Resolver_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Target_Semantic_Validator_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(Devilz_Den_Native_Manager_Test PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()
