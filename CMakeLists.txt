cmake_minimum_required(VERSION 3.16)
project(spelling_blizzard_recovered C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(recovered_message_pump STATIC
    src/recovered/framework/message_pump.c
)

add_library(recovered_application STATIC
    src/recovered/application/nested_field.c
    src/recovered/application/application_state.c
    src/recovered/application/nested_entries.c
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

add_executable(test_recovered_1018_aeaa tests/test_recovered_1018_aeaa.c)
target_link_libraries(test_recovered_1018_aeaa PRIVATE recovered_application)
add_test(NAME recovered_1018_aeaa COMMAND test_recovered_1018_aeaa)

add_executable(test_recovered_0x2af9_state tests/test_recovered_0x2af9_state.c)
target_link_libraries(test_recovered_0x2af9_state PRIVATE recovered_application)
add_test(NAME recovered_0x2af9_state COMMAND test_recovered_0x2af9_state)

add_executable(test_recovered_1018_a704 tests/test_recovered_1018_a704.c)
target_link_libraries(test_recovered_1018_a704 PRIVATE recovered_application)
add_test(NAME recovered_1018_a704 COMMAND test_recovered_1018_a704)
