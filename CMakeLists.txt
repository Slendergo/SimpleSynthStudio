cmake_minimum_required(VERSION 3.21)

project(SimpleSynthStudio VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SIMPLESYNTH_FETCH_DEPENDENCIES "Download GLFW and OpenAL Soft when they are not installed" ON)
option(SIMPLESYNTH_STATIC_RUNTIME "Link the MSVC runtime statically" ON)

if(MSVC AND SIMPLESYNTH_STATIC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 QUIET)
find_package(OpenAL QUIET)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(NOT TARGET glfw)
    if(NOT SIMPLESYNTH_FETCH_DEPENDENCIES)
        message(FATAL_ERROR "GLFW was not found. Install it or enable SIMPLESYNTH_FETCH_DEPENDENCIES.")
    endif()
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(glfw)
endif()

if(NOT TARGET OpenAL::OpenAL)
    if(NOT SIMPLESYNTH_FETCH_DEPENDENCIES)
        message(FATAL_ERROR "OpenAL was not found. Install it or enable SIMPLESYNTH_FETCH_DEPENDENCIES.")
    endif()
    set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(openal URL https://github.com/kcat/openal-soft/archive/refs/tags/1.25.2.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(openal)
    if(NOT TARGET OpenAL::OpenAL)
        add_library(OpenAL::OpenAL ALIAS OpenAL)
    endif()
endif()

add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC vendor/imgui vendor/imgui/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL ${CMAKE_DL_LIBS})

add_executable(SimpleSynthStudio
    src/Main.cpp
    src/Audio/AudioBuffer.cpp
    src/Audio/AudioEngine.cpp
    src/Audio/Synth.cpp
    src/Audio/SynthLibrary.cpp
    src/Audio/WavFile.cpp
    src/Audio/ZipFile.cpp
    src/SimpleSynthStudio/FileDialog.cpp
    src/SimpleSynthStudio/LibraryExporter.cpp
    src/SimpleSynthStudio/ProjectExporter.cpp
    src/SimpleSynthStudio/SynthApp.cpp
    src/SimpleSynthStudio/SynthEditor.cpp
    src/SimpleSynthStudio/SynthHistory.cpp
    src/SimpleSynthStudio/SynthProject.cpp
)

target_include_directories(SimpleSynthStudio PRIVATE src)
target_precompile_headers(SimpleSynthStudio PRIVATE src/pch.h)
target_link_libraries(SimpleSynthStudio PRIVATE imgui glfw OpenAL::OpenAL OpenGL::GL)

if(OPENAL_INCLUDE_DIR)
    get_filename_component(SIMPLESYNTH_OPENAL_PARENT "${OPENAL_INCLUDE_DIR}" DIRECTORY)
    target_include_directories(SimpleSynthStudio PRIVATE "${OPENAL_INCLUDE_DIR}" "${SIMPLESYNTH_OPENAL_PARENT}")
endif()

if(WIN32)
    target_link_libraries(SimpleSynthStudio PRIVATE comdlg32 ole32 shell32 uuid)
endif()

if(MSVC)
    target_compile_options(SimpleSynthStudio PRIVATE /utf-8 /Zc:__cplusplus /Zc:preprocessor /MP)
    target_compile_definitions(SimpleSynthStudio PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

if(WIN32)
    add_custom_command(TARGET SimpleSynthStudio POST_BUILD COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:SimpleSynthStudio>>,copy_if_different,true> $<TARGET_RUNTIME_DLLS:SimpleSynthStudio> $<TARGET_FILE_DIR:SimpleSynthStudio> COMMAND_EXPAND_LISTS)
endif()

install(TARGETS SimpleSynthStudio RUNTIME DESTINATION .)
install(FILES README.md DESTINATION .)
