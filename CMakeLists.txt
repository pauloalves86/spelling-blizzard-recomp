cmake_minimum_required(VERSION 3.16)
project(spelling_blizzard_recovered C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(sb_win16_shim STATIC
    src/shim/win16_resource.c
)

add_library(recovered_message_pump STATIC
    src/recovered/framework/message_pump.c
)

add_library(recovered_resource_manager STATIC
    src/recovered/framework/resource_manager.c
)
target_link_libraries(recovered_resource_manager PUBLIC sb_win16_shim)

add_library(recovered_application STATIC
    src/recovered/application/object_model.c
    src/recovered/application/application_state.c
)

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
