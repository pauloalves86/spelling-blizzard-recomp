cmake_minimum_required(VERSION 3.16)
project(spelling_blizzard_recomp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)

# ---------------------------------------------------------------------------
# Host runtime: CPU helpers, loader and the Win16 API layer. It contains no
# game code and builds without the game files.
# ---------------------------------------------------------------------------
# (Object files, not an archive: the generated code's weak API stubs must
# never win over the runtime's real implementations.)
add_library(sb_recomp_runtime OBJECT
    src/recomp/main.c src/recomp/runtime.c src/recomp/loader.c src/recomp/kernel.c
    src/recomp/hostfs.c src/recomp/user.c src/recomp/gdi.c src/recomp/mmsystem.c)
target_include_directories(sb_recomp_runtime PUBLIC src/recomp src/recomp/imports)
target_link_libraries(sb_recomp_runtime PUBLIC SDL2::SDL2 PkgConfig::FREETYPE)
target_compile_options(sb_recomp_runtime PRIVATE -Wall -Wno-unused-function)

set(SB_GAME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/original CACHE PATH "Folder with your BLIZZARD.EXE and SB_CDRES.DLL")
if(NOT EXISTS ${SB_GAME_DIR}/BLIZZARD.EXE)
    message(WARNING "${SB_GAME_DIR}/BLIZZARD.EXE not found: building the runtime only. "
                    "Copy your game files into original/ to build sb_recomp (see README.md).")
    return()
endif()

# ---------------------------------------------------------------------------
# Static recompilation of original/BLIZZARD.EXE (tools/recomp/gen.py).
# Generated sources live in the build tree and are never distributed.
# ---------------------------------------------------------------------------
set(RECOMP_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/recomp_gen)
file(GLOB RECOMP_TOOL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tools/recomp/*.py
                              ${CMAKE_CURRENT_SOURCE_DIR}/tools/recomp/*.txt)
execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/recomp/gen.py ${RECOMP_GEN_DIR}
                        --exe ${SB_GAME_DIR}/BLIZZARD.EXE
                RESULT_VARIABLE RECOMP_GEN_RESULT)
if(NOT RECOMP_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "tools/recomp/gen.py failed")
endif()
file(GLOB RECOMP_GEN_SOURCES ${RECOMP_GEN_DIR}/gen_*.c)
add_custom_command(OUTPUT ${RECOMP_GEN_DIR}/stamp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/recomp/gen.py ${RECOMP_GEN_DIR}
            --exe ${SB_GAME_DIR}/BLIZZARD.EXE
    COMMAND ${CMAKE_COMMAND} -E touch ${RECOMP_GEN_DIR}/stamp
    DEPENDS ${RECOMP_TOOL_SOURCES} ${SB_GAME_DIR}/BLIZZARD.EXE
    COMMENT "Recompiling original/BLIZZARD.EXE to C")
add_library(sb_recomp_gen STATIC ${RECOMP_GEN_SOURCES} ${RECOMP_GEN_DIR}/stamp)
target_include_directories(sb_recomp_gen PRIVATE ${RECOMP_GEN_DIR} src/recomp src/recomp/imports)
target_compile_options(sb_recomp_gen PRIVATE -O1 -w)

add_executable(sb_recomp $<TARGET_OBJECTS:sb_recomp_runtime>)
target_link_libraries(sb_recomp PRIVATE sb_recomp_gen SDL2::SDL2 PkgConfig::FREETYPE m)

enable_testing()
add_test(NAME recomp_new_game_headless
         COMMAND sb_recomp --headless --no-audio --game-dir ${SB_GAME_DIR} --data-dir ${CMAKE_CURRENT_BINARY_DIR}/recomp_test_run
                 --script ${CMAKE_CURRENT_SOURCE_DIR}/tests/recomp/new_game.script
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(recomp_new_game_headless PROPERTIES TIMEOUT 120)
