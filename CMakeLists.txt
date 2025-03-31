# You can add your .cpp files at sources.cmake
include(sources.cmake)

target_include_directories(decoder_faster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
link_decoder_deps(decoder_faster)
target_link_libraries(test_decoder_faster decoder_faster)
target_compile_definitions(test_decoder_faster PUBLIC HSE_ARTIFACTS_DIR="/home/renedyn/proga/cpp-advanced-hse/tasks/jpeg-decoder/faster/my_image")
