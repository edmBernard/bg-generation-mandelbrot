cmake_minimum_required(VERSION 3.21)

#**************************************************************************************************
# Project *****************************************************************************************
project(bg-generation-mandelbrot
        HOMEPAGE_URL https://github.com/edmBernard/bg-generation-mandelbrot)

#**************************************************************************************************
# Find Package ************************************************************************************
find_package(cxxopts REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

find_path(STB_INCLUDE_DIRS "stb_image_write.h")
find_path(LINMATH_INCLUDE_DIRS "linmath.h/linmath.h")

#**************************************************************************************************
# Include *****************************************************************************************
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${STB_INCLUDE_DIRS})
include_directories(${LINMATH_INCLUDE_DIRS})

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

#**************************************************************************************************
# Set compiler ************************************************************************************
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  add_compile_options(/Oy /utf-8 /Zc:__cplusplus /DUNICODE /D_UNICODE)
else()
  add_compile_options(-fPIC -O3)
endif()

#**************************************************************************************************
# Set variable ************************************************************************************
SET(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

#**************************************************************************************************
# Make configuration ******************************************************************************
add_executable(bg-generation-mandelbrot ${SOURCES})
target_link_libraries(bg-generation-mandelbrot PRIVATE fmt::fmt-header-only spdlog::spdlog_header_only cxxopts::cxxopts)
target_link_libraries(bg-generation-mandelbrot PRIVATE glfw glad::glad)

file(GLOB SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.glsl)

add_custom_target(
  copy_shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:bg-generation-mandelbrot>/shaders
  DEPENDS SHADER_FILES
)
add_dependencies(bg-generation-mandelbrot copy_shaders)