cmake_minimum_required(VERSION 3.16)
project(spelling_blizzard_recomp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

# ---------------------------------------------------------------------------
# The recompiler (src/recomp/gen): BLIZZARD.EXE -> C. No game code in it either.
# ---------------------------------------------------------------------------
set(SB_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen)
set(SB_GEN_SOURCES ${SB_GEN_DIR}/gen.c ${SB_GEN_DIR}/ne.c ${SB_GEN_DIR}/x86.c ${SB_GEN_DIR}/exports.c)
# functions.txt, built in as a string
set(SB_GEN_FUNCTIONS_C ${CMAKE_CURRENT_BINARY_DIR}/gen_functions_txt.c)
file(READ ${SB_GEN_DIR}/functions.txt SB_GEN_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," SB_GEN_HEX "${SB_GEN_HEX}")
file(WRITE ${SB_GEN_FUNCTIONS_C}.new
     "/* Generated from src/recomp/gen/functions.txt. */\nconst char gen_functions_txt[] = {${SB_GEN_HEX}0};\n")
configure_file(${SB_GEN_FUNCTIONS_C}.new ${SB_GEN_FUNCTIONS_C} COPYONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SB_GEN_DIR}/functions.txt)

add_library(sb_gen_lib STATIC ${SB_GEN_SOURCES} ${SB_GEN_FUNCTIONS_C})
target_include_directories(sb_gen_lib PUBLIC ${SB_GEN_DIR})
target_compile_options(sb_gen_lib PRIVATE -Wall -Wextra)
add_executable(sb_gen ${SB_GEN_DIR}/sb_gen.c)
target_link_libraries(sb_gen PRIVATE sb_gen_lib)
target_compile_options(sb_gen PRIVATE -Wall -Wextra)

set(SB_GAME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/original CACHE PATH "Folder with your BLIZZARD.EXE and SB_CDRES.DLL")
if(NOT EXISTS ${SB_GAME_DIR}/BLIZZARD.EXE)
    message(WARNING "${SB_GAME_DIR}/BLIZZARD.EXE not found: building the runtime only. "
                    "Copy your game files into original/ to build sb_recomp (see README.md).")
    return()
endif()

# ---------------------------------------------------------------------------
# Static recompilation of BLIZZARD.EXE into the build tree (never distributed).
# The generated file names depend on the EXE, so the recompiler is built and
# run while configuring; changing the EXE or the recompiler configures again.
# ---------------------------------------------------------------------------
set(RECOMP_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/recomp_gen)
try_compile(SB_GEN_BUILT ${CMAKE_CURRENT_BINARY_DIR}/sb_gen_configure
    SOURCES ${SB_GEN_SOURCES} ${SB_GEN_DIR}/sb_gen.c ${SB_GEN_FUNCTIONS_C}
    COMPILE_DEFINITIONS -O2
    C_STANDARD 11
    OUTPUT_VARIABLE SB_GEN_LOG
    COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/sb_gen_configure/sb_gen)
if(NOT SB_GEN_BUILT)
    message(FATAL_ERROR "Building the recompiler failed:\n${SB_GEN_LOG}")
endif()
execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/sb_gen_configure/sb_gen ${RECOMP_GEN_DIR}
                        --exe ${SB_GAME_DIR}/BLIZZARD.EXE --imports ${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/imports
                RESULT_VARIABLE RECOMP_GEN_RESULT)
if(NOT RECOMP_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "Recompiling ${SB_GAME_DIR}/BLIZZARD.EXE failed")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
             ${SB_GEN_SOURCES} ${SB_GEN_DIR}/sb_gen.c ${SB_GEN_DIR}/gen.h ${SB_GEN_DIR}/ne.h ${SB_GEN_DIR}/x86.h
             ${SB_GEN_DIR}/exports.h ${SB_GAME_DIR}/BLIZZARD.EXE)
file(GLOB RECOMP_GEN_SOURCES ${RECOMP_GEN_DIR}/gen_*.c)
add_library(sb_recomp_gen STATIC ${RECOMP_GEN_SOURCES})
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
