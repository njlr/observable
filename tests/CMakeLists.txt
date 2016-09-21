add_subdirectory(shared_lib)

include(compile_flags)
include(observable_includes)
include(source_groups)

set(SOURCE_FILES src/detail/callable_collection.cpp
                 src/detail/unique_subscription.cpp
                 src/detail/shared_subscription.cpp
                 src/gtest.h
                 src/compound_subject.cpp
                 src/property.cpp
                 src/shared_lib.cpp
                 src/subject.cpp)

add_executable(tests ${SOURCE_FILES})

configure_compiler(tests)
add_observable_includes(tests)
target_link_libraries(tests shared gtest gtest_main)
target_include_directories(tests PRIVATE src shared_lib)
target_include_directories(tests
                           SYSTEM
                           PRIVATE ${PROJECT_SOURCE_DIR}/external/gtest/include)

add_custom_command(
    TARGET tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:shared> $<TARGET_FILE_DIR:tests>
)

add_test(NAME tests
         COMMAND tests
         WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)