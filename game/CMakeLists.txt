add_executable(game src/main.cpp)

target_link_libraries(game PRIVATE engine project_warnings)

# Absolute path to source root so assets are found regardless of CWD
target_compile_definitions(game PRIVATE
    ASSET_ROOT="${CMAKE_SOURCE_DIR}"
)

# Copy shaders next to binary for convenience (e.g., running from build dir)
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:game>/shaders"
    COMMENT "Syncing shaders to build output"
)

# Copy assets next to binary (ASSET_ROOT macro still used for absolute paths)
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:game>/assets"
    COMMENT "Syncing assets to build output"
)
