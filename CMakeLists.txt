cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(Greenwake LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Greenwake still uses Windows GDI+ for images/fonts. Build on Windows with MSVC.")
endif()

# Match the standalone Visual Studio project; no VC runtime DLL is required.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "Use static MSVC runtime" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(SimpleGame/Dependencies/glfw EXCLUDE_FROM_ALL)

add_library(glew STATIC SimpleGame/Dependencies/glew/src/glew.c)
target_include_directories(glew PUBLIC SimpleGame/Dependencies/glew/include)
target_compile_definitions(glew PUBLIC GLEW_STATIC)
target_link_libraries(glew PUBLIC opengl32)

set(GAME_SOURCES
    SimpleGame/Actor.cpp
    SimpleGame/CraftingSystem.cpp
    SimpleGame/GameContent.cpp
    SimpleGame/GameInput.cpp
    SimpleGame/GameManager.cpp
    SimpleGame/GameState.cpp
    SimpleGame/GameUI.cpp
    SimpleGame/ModelCache.cpp
    SimpleGame/Monster.cpp
    SimpleGame/PlayerController.cpp
    SimpleGame/QuestSystem.cpp
    SimpleGame/Renderer.cpp
    SimpleGame/StorySystem.cpp
    SimpleGame/WorldRenderer.cpp)
add_library(game STATIC ${GAME_SOURCES})
target_compile_features(game PUBLIC cxx_std_17)
target_include_directories(game PUBLIC SimpleGame)
target_compile_definitions(game PUBLIC NOMINMAX UNICODE _UNICODE GLFW_INCLUDE_NONE)
target_link_libraries(game PUBLIC glew glfw gdiplus)
if(MSVC)
    target_compile_options(game PUBLIC /utf-8)
endif()

add_custom_target(game_runtime
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/tools/CopyRuntime.ps1"
        -Source "${CMAKE_SOURCE_DIR}/SimpleGame"
        -Destination "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    VERBATIM)

function(configure_game_runtime target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>")
    # One shared copy step avoids concurrent test targets writing the same files.
    add_dependencies(${target} game_runtime)
endfunction()

add_executable(SimpleGame SimpleGame/SimpleGame.cpp)
target_link_libraries(SimpleGame PRIVATE game)
configure_game_runtime(SimpleGame)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT SimpleGame)

# Building and running tests are separate, explicit actions. Smoke tests can capture images.
option(GSE_BUILD_TESTS "Build checks and renderer smoke executables (does not run them)" OFF)
if(GSE_BUILD_TESTS)
    foreach(check AnimalHuntingCheck ChatacabraCheck GameContentCheck GlfwWindowCheck HeroAttackSmoke HudLayoutCheck InventoryCraftingCheck NpcWanderCheck
                  RenderQualitySmoke StoryFlowCheck TerrainCacheSmoke WorldLayoutCheck)
        add_executable(${check} tests/${check}.cpp)
        target_link_libraries(${check} PRIVATE game)
        if(MSVC)
            target_compile_options(${check} PRIVATE /UNDEBUG)
        endif()
        configure_game_runtime(${check})
    endforeach()
endif()
