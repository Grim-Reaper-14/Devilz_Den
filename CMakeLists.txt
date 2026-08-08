cmake_minimum_required(VERSION 3.24)
project(Devilz_Den LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE DEVILZ_DEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(Devilz_Den ${DEVILZ_DEN_SOURCES})

target_include_directories(Devilz_Den PRIVATE src)
target_compile_definitions(Devilz_Den PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if(WIN32)
    target_link_libraries(Devilz_Den PRIVATE psapi)
endif()

if(MSVC)
    target_compile_options(Devilz_Den PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()
