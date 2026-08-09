cmake_minimum_required(VERSION 3.24)
project(Devilz_Den LANGUAGES CXX)

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
endif()

if(MSVC)
    target_compile_options(Devilz_Den PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()
