cmake_minimum_required(VERSION 3.16)
project(spelling_blizzard_recovered C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(recovered_message_pump STATIC
    src/recovered/framework/message_pump.c
)

add_library(recovered_application STATIC
    src/recovered/application/object_model.c
    src/recovered/application/application_state.c
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
