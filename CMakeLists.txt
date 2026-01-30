cmake_minimum_required(VERSION 3.25)
project(Runa VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(USE_FOLDERS ON)
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

include(engine/engine.cmake)