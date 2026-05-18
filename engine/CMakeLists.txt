add_library(engine STATIC
    src/core/Window.cpp
    src/core/App.cpp
    src/render/Shader.cpp
    src/render/Texture.cpp
    src/render/Camera.cpp
    src/render/SpriteBatch.cpp
    src/render/Mesh.cpp
    src/render/MeshRenderer.cpp
    src/render/ParallaxRenderer.cpp
    src/ecs/Registry.cpp
    src/input/InputManager.cpp
    src/physics/PhysicsWorld.cpp
    src/physics/DebugDraw.cpp
    src/animation/AnimationClip.cpp
    src/map/TileMap.cpp
    src/map/TileMapRenderer.cpp
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
        box2d
        nlohmann_json::nlohmann_json
    PRIVATE
        Freetype::Freetype
        stb
        assimp::assimp
        pugixml::pugixml
)

if(ENABLE_IMGUI AND TARGET imgui_lib)
    target_link_libraries(engine PUBLIC imgui::imgui)
    target_compile_definitions(engine PUBLIC ENGINE_IMGUI=1)
endif()

target_link_libraries(engine PRIVATE project_warnings)
target_compile_features(engine PUBLIC cxx_std_20)
