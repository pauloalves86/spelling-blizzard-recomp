cmake_minimum_required(VERSION 3.16)
project(spelling_blizzard_recovered C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(sb_win16_shim STATIC
    src/shim/win16_resource.c
    src/shim/win16_waveout.c
    src/shim/win16_gdi.c
    src/shim/win16_user.c
)

add_library(recovered_message_pump STATIC
    src/recovered/framework/message_pump.c
)

add_library(recovered_resource_manager STATIC
    src/recovered/framework/resource_manager.c
    src/recovered/framework/audio_waveout.c
    src/recovered/framework/graphics_gdi.c
    src/recovered/framework/ui_framework.c
)
target_link_libraries(recovered_resource_manager PUBLIC sb_win16_shim)

add_library(recovered_application STATIC
    src/recovered/application/object_model.c
    src/recovered/application/application_state.c
    src/recovered/application/game_logic.c
)
target_link_libraries(recovered_application PUBLIC recovered_resource_manager)

add_custom_target(win16_objects
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_win16_objects.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building reconstructed C as a Win16 OMF library with Open Watcom"
)
target_include_directories(recovered_message_pump PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recovered/framework
)
target_include_directories(recovered_application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recovered/application
)

enable_testing()
add_executable(test_message_pump tests/test_message_pump.c)
target_link_libraries(test_message_pump PRIVATE recovered_message_pump)
add_test(NAME recovered_message_pump COMMAND test_message_pump)

add_executable(test_recovered_object_model tests/test_recovered_object_model.c)
target_link_libraries(test_recovered_object_model PRIVATE recovered_application)
add_test(NAME recovered_object_model COMMAND test_recovered_object_model)

add_executable(test_resource_manager tests/test_resource_manager.c)
target_link_libraries(test_resource_manager PRIVATE recovered_resource_manager)
add_test(NAME recovered_resource_manager COMMAND test_resource_manager WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_audio_waveout tests/test_audio_waveout.c)
target_link_libraries(test_audio_waveout PRIVATE recovered_resource_manager)
add_test(NAME recovered_audio_waveout COMMAND test_audio_waveout WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_graphics_gdi tests/test_graphics_gdi.c)
target_link_libraries(test_graphics_gdi PRIVATE recovered_resource_manager)
add_test(NAME recovered_graphics_gdi COMMAND test_graphics_gdi WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_ui_framework tests/test_ui_framework.c)
target_link_libraries(test_ui_framework PRIVATE recovered_resource_manager)
add_test(NAME recovered_ui_framework COMMAND test_ui_framework WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_game_logic tests/test_game_logic.c)
target_link_libraries(test_game_logic PRIVATE recovered_application)
add_test(NAME recovered_game_logic COMMAND test_game_logic WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(spelling_blizzard src/main.c)
target_link_libraries(spelling_blizzard PRIVATE recovered_application)
add_test(NAME spelling_blizzard_runner
         COMMAND spelling_blizzard --level 1 --save-ppm ${CMAKE_CURRENT_BINARY_DIR}/frame_preview.ppm
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Optional SDL2 host platform backend: real window, input, audio and clock
# beneath the Win16 shim. Enables `spelling_blizzard --window`.
option(SB_WITH_SDL2 "Build the SDL2 platform backend (--window mode)" ON)
if(SB_WITH_SDL2)
    find_package(SDL2 CONFIG QUIET)
    if(SDL2_FOUND)
        add_library(sb_platform_sdl STATIC src/platform/sb_platform_sdl.c)
        target_link_libraries(sb_platform_sdl PUBLIC sb_win16_shim SDL2::SDL2)
        target_link_libraries(spelling_blizzard PRIVATE sb_platform_sdl)
        target_compile_definitions(spelling_blizzard PRIVATE SB_HAVE_PLATFORM=1)
        # Headless smoke test of the windowed loop using SDL's dummy drivers.
        add_test(NAME spelling_blizzard_window_headless
                 COMMAND spelling_blizzard --window --frames 30
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(spelling_blizzard_window_headless PROPERTIES
                 ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
        add_executable(test_platform_sdl tests/test_platform_sdl.c)
        target_link_libraries(test_platform_sdl PRIVATE sb_platform_sdl)
        add_test(NAME platform_sdl_input COMMAND test_platform_sdl)
        message(STATUS "SDL2 platform backend enabled (spelling_blizzard --window)")
    else()
        message(STATUS "SDL2 not found: building without --window support")
    endif()
endif()
