add_executable(hotword_tests
    hotword_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/asr/hotword_utils.cpp
)

target_include_directories(hotword_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hotword_tests PRIVATE common)

add_test(NAME hotword_tests COMMAND hotword_tests)

add_executable(prompt_template_tests
    prompt_template_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/postprocess/prompt_template.cpp
)

target_include_directories(prompt_template_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME prompt_template_tests COMMAND prompt_template_tests)
