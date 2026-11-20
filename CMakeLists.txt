cmake_minimum_required(VERSION 3.16)

project(base_glfw LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(base_glfw
    src/main.cpp
    src/lsystem.cpp
    src/util.cpp
    src/gl_core_3_3.c
)

target_include_directories(base_glfw PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(base_glfw PRIVATE
    glfw
    glm::glm
    OpenGL::GL
)
