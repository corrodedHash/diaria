add_test(NAME lit_end-to-end COMMAND env buildpath=${CMAKE_BINARY_DIR} lit -vvv  . WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
