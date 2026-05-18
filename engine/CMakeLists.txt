add_library(engine STATIC
    src/core/Window.cpp
    src/core/App.cpp
    src/render/Shader.cpp
)

target_include_directories(engine
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(engine
    PUBLIC
        SDL2::SDL2
        OpenGL::GL
        GLEW::GLEW
        glm::glm
    PRIVATE
        Freetype::Freetype
)

if(ENABLE_IMGUI AND TARGET imgui_lib)
    target_link_libraries(engine PUBLIC imgui::imgui)
    target_compile_definitions(engine PUBLIC ENGINE_IMGUI=1)
endif()

target_link_libraries(engine PRIVATE project_warnings)
target_compile_features(engine PUBLIC cxx_std_20)
