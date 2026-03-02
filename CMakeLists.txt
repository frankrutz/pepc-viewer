cmake_minimum_required(VERSION 3.16)
project(pepc-viewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependencies (ImGui + ImPlot via FetchContent)
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking   # latest docking branch
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(implot)

# ---------------------------------------------------------------------------
# Platform detection: Emscripten (WASM) vs. native desktop
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    set(BACKEND_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    set(PLATFORM_COMPILE_OPTIONS
        -sUSE_SDL=2
    )
    set(PLATFORM_LINK_OPTIONS
        -sUSE_SDL=2
        -sUSE_WEBGL2=1
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
        --shell-file "${CMAKE_SOURCE_DIR}/shell.html"
    )
    set(OUTPUT_SUFFIX ".html")
else()
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)

    set(BACKEND_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    set(PLATFORM_LIBS
        SDL2::SDL2
        OpenGL::GL
    )
    set(OUTPUT_SUFFIX "")
endif()

# ---------------------------------------------------------------------------
# ImGui static library
# ---------------------------------------------------------------------------
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${BACKEND_SOURCES}
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
if(EMSCRIPTEN)
    target_compile_options(imgui_lib PUBLIC ${PLATFORM_COMPILE_OPTIONS})
else()
    target_include_directories(imgui_lib PUBLIC ${SDL2_INCLUDE_DIRS})
endif()

# ---------------------------------------------------------------------------
# ImPlot static library
# ---------------------------------------------------------------------------
add_library(implot_lib STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot_lib PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot_lib PUBLIC imgui_lib)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(pepc-viewer src/main.cpp)
target_include_directories(pepc-viewer PRIVATE src)
target_link_libraries(pepc-viewer PRIVATE imgui_lib implot_lib)

if(EMSCRIPTEN)
    target_compile_options(pepc-viewer PRIVATE ${PLATFORM_COMPILE_OPTIONS})
    set_target_properties(pepc-viewer PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "${PLATFORM_LINK_OPTIONS}"
    )
else()
    target_link_libraries(pepc-viewer PRIVATE ${PLATFORM_LIBS})
endif()
