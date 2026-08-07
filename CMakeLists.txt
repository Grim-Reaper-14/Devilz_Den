cmake_minimum_required(VERSION 3.24)
project(Devilz_Den LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Devilz_Den
    src/main.cpp
    src/Backend/Error/Error.cpp
    src/Backend/Logging/LoggerService.cpp
    src/Backend/Logging/Sinks/DebuggerSink.cpp
    src/Backend/Logging/Sinks/FileSink.cpp
    src/Backend/Threading/ThreadManager.cpp
)

target_include_directories(Devilz_Den PRIVATE src)
target_compile_definitions(Devilz_Den PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    target_compile_options(Devilz_Den PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()
