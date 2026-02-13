cmake_minimum_required(VERSION 3.10)
project(talawachess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Threading support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Force static linking when cross-compiling with MinGW
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# OPTIMIZATION FLAGS
if(MSVC)
    add_compile_options(/O2)
else()
    add_compile_options(-O3 -march=native -Wall)
endif()
# Include the 'include' directory
include_directories(include)

# ---------------------------------------------------------
# AUTO-DISCOVERY
# This finds all .cpp files in src/ and saves them to SOURCES
# ---------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the executable using the discovered list
add_executable(talawachess ${SOURCES})
target_link_libraries(talawachess PRIVATE Threads::Threads)