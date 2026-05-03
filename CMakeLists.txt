cmake_minimum_required(VERSION 3.16)
project(VulkanEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies ──────────────────────────────────────────────
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client)

# ── Sources ───────────────────────────────────────────────────
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB WAYLAND_PROTOCOL_SOURCES CONFIGURE_DEPENDS src/platfroms/wayland/*-protocol.c)
list(APPEND ENGINE_SOURCES ${WAYLAND_PROTOCOL_SOURCES})

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_executable(VulkanOut main.cpp ${ENGINE_SOURCES} ${IMGUI_SOURCES})

target_include_directories(VulkanOut PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(VulkanOut PRIVATE
    Vulkan::Vulkan
    glfw
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${WAYLAND_LIBRARIES}
)

# X11 libs on Linux
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(VulkanOut PRIVATE X11::X11 X11::Xrandr X11::Xi)
    target_link_libraries(VulkanOut PRIVATE Xxf86vm)
endif()

# ── Shader compilation ────────────────────────────────────────
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found – install the Vulkan SDK")
endif()

file(GLOB SHADER_SOURCES
    ${CMAKE_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_SOURCE_DIR}/shaders/*.comp
)

foreach(SHADER ${SHADER_SOURCES})
    set(SPV "${SHADER}.spv")
    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER}"
    )
    list(APPEND SPV_SHADERS ${SPV})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_SHADERS})
add_dependencies(VulkanOut shaders)
