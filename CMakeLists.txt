cmake_minimum_required(VERSION 3.16)
project(Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(game
  src/main.cpp
  src/Game.cpp
  src/Zoom.cpp
  src/Scene.h
  src/MenuScene.cpp
  src/OptionsScene.cpp

  # gameplay
  src/GameScene.cpp

  src/Text.cpp
)

target_include_directories(game PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ------------------------------------------------------------
# Emscripten (Web)
# ------------------------------------------------------------
if(EMSCRIPTEN)

  # Output as index.html for easy hosting
  set_target_properties(game PROPERTIES
    OUTPUT_NAME "index"
    SUFFIX ".html"
  )

  # SDL2 + SDL2_image + SDL2_ttf via Emscripten ports
  target_compile_options(game PRIVATE
    -O2
    -sUSE_SDL=2
    -sUSE_SDL_IMAGE=2
    -sUSE_SDL_TTF=2
    -sSDL2_IMAGE_FORMATS=png
  )

  target_link_options(game PRIVATE
    -O2
    -sUSE_SDL=2
    -sUSE_SDL_IMAGE=2
    -sUSE_SDL_TTF=2

    # PNG support for SDL2_image (use comma-separated list for ports script)
    -sSDL2_IMAGE_FORMATS=png

    # Make the build more forgiving with memory
    -sALLOW_MEMORY_GROWTH=1

    # Useful for many SDL2 apps (safe default)
    -sASYNCIFY=1

    # Package assets/ into the virtual FS at /assets
    --preload-file "${CMAKE_SOURCE_DIR}/assets@/assets"
  )

# ------------------------------------------------------------
# Native (Linux/WSL/etc.)
# ------------------------------------------------------------
else()

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2)
  pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)
  pkg_check_modules(SDL2IMAGE REQUIRED SDL2_image)

  target_include_directories(game PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
  )

  target_link_libraries(game PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2TTF_LIBRARIES}
    ${SDL2IMAGE_LIBRARIES}
  )

  target_compile_options(game PRIVATE
    ${SDL2_CFLAGS_OTHER}
    ${SDL2TTF_CFLAGS_OTHER}
    ${SDL2IMAGE_CFLAGS_OTHER}
  )

endif()
