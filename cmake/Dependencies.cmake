include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ─── System packages ──────────────────────────────────────────────────────────
# SDL2 — Fedora 43 ships sdl2-compat-devel (SDL2 API on top of SDL3).
# Try CMake config first; fall back to pkg-config (both paths work with sdl2-compat-devel).
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND AND NOT TARGET SDL2::SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PC REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2_PC)
    message(STATUS "SDL2 found via pkg-config: ${SDL2_PC_VERSION}")
elseif(NOT TARGET SDL2::SDL2)
    # Handle legacy find_package output (SDL2_LIBRARIES / SDL2_INCLUDE_DIRS)
    add_library(SDL2::SDL2 INTERFACE IMPORTED GLOBAL)
    target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
    target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    message(STATUS "SDL2 found via legacy find_package")
else()
    message(STATUS "SDL2 found via CMake config")
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)
find_package(OpenAL REQUIRED)

# Optional — used from M2 onwards
find_package(assimp QUIET)
find_package(box2d  QUIET)

# ─── GLM — math, header-only ─────────────────────────────────────────────────
find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found on system — fetching via FetchContent")
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
    )
    set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
    set(GLM_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glm)
endif()

# ─── nlohmann/json — header-only (used from M6 save system) ──────────────────
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests       OFF CACHE INTERNAL "")
set(JSON_Install          OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# ─── stb — stb_image.h (used from M1 texture loading) ───────────────────────
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# ─── Dear ImGui — debug overlay ──────────────────────────────────────────────
if(ENABLE_IMGUI)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui_lib PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui_lib PUBLIC SDL2::SDL2 OpenGL::GL GLEW::GLEW)
    # Silence warnings from third-party imgui sources
    target_compile_options(imgui_lib PRIVATE -w)
    add_library(imgui::imgui ALIAS imgui_lib)
endif()
