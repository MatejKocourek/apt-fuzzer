# CMakeList.txt : CMake project for code-coverage, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.2)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("code-coverage")


# Add source to this project's executable.
add_executable (code-coverage "code-coverage.cpp")

add_library(tree-sitter tree-sitter/lib/src/lib.c)
target_include_directories(tree-sitter PUBLIC tree-sitter/lib/src tree-sitter/lib/include)

add_library(tree-sitter-c tree-sitter-c/src/parser.c)
target_include_directories(tree-sitter-c PUBLIC tree-sitter-c/src)

target_include_directories(tree-sitter PUBLIC tree-sitter/lib/src tree-sitter/lib/include)

target_include_directories(code-coverage PUBLIC tree-sitter-c/src cpp-tree-sitter/include)
target_link_libraries(code-coverage PRIVATE tree-sitter tree-sitter-c)

set_property(TARGET code-coverage PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tests

#set(COVERAGE ON)

# If coverage is enabled, modify the compile flags
if(COVERAGE)
    message(STATUS "Code coverage enabled")


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable (tests "test.cpp")
target_include_directories(tests PUBLIC tree-sitter-c/src cpp-tree-sitter/include)
target_link_libraries(tests PRIVATE tree-sitter tree-sitter-c GTest::gtest_main)

set_property(TARGET tests PROPERTY CXX_STANDARD 20)

include(GoogleTest)
gtest_discover_tests(tests)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "GCC detected, generating code coverage")

    # Add compiler flags for coverage
    target_compile_options(tests PRIVATE -fprofile-arcs -ftest-coverage -g -O0)
    target_link_libraries(tests PRIVATE gcov)
    target_link_options(tests PRIVATE -fprofile-arcs -ftest-coverage)

    # Add a custom target to generate coverage report using lcov
    add_custom_target(coverage
        COMMAND ./tests
        COMMAND lcov --capture --directory . --output-file coverage.info --include */FileInstrument.h
        COMMAND genhtml coverage.info --output-directory coverage_report
        COMMENT "Generating code coverage report..."
        DEPENDS tests
    )
else()
    message(STATUS "Non-GCC compiler detected, cannot generate code coverage")
endif()

else()
    message(STATUS "Code coverage disabled")
endif()




