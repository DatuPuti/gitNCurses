cmake_minimum_required(VERSION 3.10)
project(gitNCurses)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# Add executable
add_executable(gitNCurses 
    src/main.cpp
    src/GitCommandHandler.cpp
)

# Link libraries
target_link_libraries(gitNCurses ${CURSES_LIBRARIES}) 