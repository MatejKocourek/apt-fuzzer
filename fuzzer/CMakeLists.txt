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

#include(CheckIPOSupported)
#check_ipo_supported(RESULT supported OUTPUT error)

#if( supported )
    #message(STATUS "IPO / LTO enabled")
    #set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#else()
    #message(STATUS "IPO / LTO not supported: <${error}>")
#endif()

if (MSVC)
    add_compile_options(/arch:AVX512)
else()
    add_compile_options(-march=native)
endif()

# Add source to this project's executable.
add_executable (fuzzer "fuzzer.cpp" "median.h")

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