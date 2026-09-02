add_executable(hotword_tests
    hotword_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/asr/hotword_utils.cpp
)

target_include_directories(hotword_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hotword_tests PRIVATE common)

add_test(NAME hotword_tests COMMAND hotword_tests)

add_executable(core_config_tests
    core_config_tests.cpp
)

target_include_directories(core_config_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(core_config_tests PRIVATE config)

add_test(NAME core_config_tests COMMAND core_config_tests)

add_executable(recognition_pipeline_tests
    recognition_pipeline_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/runtime/recognition_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/postprocess/post_processor.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/postprocess/prompt_template.cpp
)

target_include_directories(recognition_pipeline_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(recognition_pipeline_tests PRIVATE common)

add_test(NAME recognition_pipeline_tests COMMAND recognition_pipeline_tests)
