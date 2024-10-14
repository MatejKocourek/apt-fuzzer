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

target_include_directories(code-coverage PUBLIC tree-sitter/lib/src tree-sitter/lib/include tree-sitter-c/src cpp-tree-sitter/include)
target_link_libraries(code-coverage PRIVATE tree-sitter tree-sitter-c)


set_property(TARGET code-coverage PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.
