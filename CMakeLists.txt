cmake_minimum_required(VERSION 3.10)
project(talawachess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the 'include' directory
include_directories(include)

# ---------------------------------------------------------
# AUTO-DISCOVERY
# This finds all .cpp files in src/ and saves them to SOURCES
# ---------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the executable using the discovered list
add_executable(talawachess.exe ${SOURCES})