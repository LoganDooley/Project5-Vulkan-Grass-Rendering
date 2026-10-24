cmake_minimum_required(VERSION 3.25)
project(vulkan_grass_rendering LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Vulkan 1.4 REQUIRED)

# Include the custom slang compiling step
include(cmake/SlangShaders.cmake)

# External packages
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(external/glm)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Headers)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.h")

add_executable(vulkan_grass_rendering ${SOURCES})

target_include_directories(vulkan_grass_rendering PRIVATE "${SRC_DIR}" "${CMAKE_SOURCE_DIR}/external/stb")

target_link_libraries(vulkan_grass_rendering PRIVATE Vulkan::Headers glfw glm::glm imgui)

target_compile_definitions(vulkan_grass_rendering PRIVATE
    VULKAN_HPP_NO_CONSTRUCTORS
    VULKAN_HPP_NO_STRUCT_SETTERS
    GLFW_INCLUDE_NONE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
    NOMINMAX
)

if(MSVC)
    target_compile_options(vulkan_grass_rendering PRIVATE /W4 /permissive-)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT vulkan_grass_rendering)
endif()

file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/shaders/*.slang")
# Call custom helper for compiling slang shaders
add_slang_shaders(vulkan_grass_rendering
    SOURCES ${SHADER_SOURCES}
    INCLUDE_DIRS "${SRC_DIR}/shaders/modules")
