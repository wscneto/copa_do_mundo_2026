cmake_minimum_required(VERSION 3.16)
project(copa_do_mundo_2026 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenAL QUIET)

add_executable(worldcup2026
    src/main.cpp
    src/Game.cpp
    src/GameInput.cpp
    src/GameSimulation.cpp
    src/GameRender.cpp
    src/AudioSystem.cpp
)

target_include_directories(worldcup2026 PRIVATE include)

target_link_libraries(worldcup2026 PRIVATE OpenGL::GL GLUT::GLUT)

if (OpenAL_FOUND)
    target_link_libraries(worldcup2026 PRIVATE OpenAL::OpenAL)
    target_compile_definitions(worldcup2026 PRIVATE HAS_OPENAL=1)
endif ()

if (UNIX AND NOT APPLE)
    target_link_libraries(worldcup2026 PRIVATE m)
endif ()
