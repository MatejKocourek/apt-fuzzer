# CMakeList.txt : CMake project for fuzzer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.9)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("fuzzer")

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if( supported )
    message(STATUS "IPO / LTO enabled for release builds")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

if (MSVC)
    add_compile_options(/arch:AVX512)
else()
    add_compile_options(-march=native)
endif()

# Add source to this project's executable.
add_executable (fuzzer "fuzzer.cpp" "median.h" "fuzzer.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET fuzzer PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

# Specify the required Boost version and components
set(Boost_VERSION 1.64.0) # Minimum required Boost version (adjust if needed)
set(Boost_USE_STATIC_LIBS ON)  # Option to link against static Boost libraries
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Set the list of required Boost libraries here
find_package(Boost ${Boost_VERSION} REQUIRED COMPONENTS filesystem) # COMPONENTS process

if(Boost_FOUND)
    message(STATUS "Found Boost ${Boost_VERSION}")
    message(STATUS "Boost include directory: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
    target_include_directories(fuzzer PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(fuzzer PRIVATE ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Boost not found!")
endif()



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
target_link_libraries(tests PRIVATE GTest::gtest_main)

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
        COMMAND lcov --capture --directory . --output-file coverage.info --include */fuzzer.h
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