#Project
cmake_minimum_required(VERSION 2.4)
project(gcsgbm)

# Directories
set (PNG_INC_DIR png/libpng)
set (PNG_LIB_DIR png)

# Include directories
include_directories(${PNG_INC_DIR})

# Library directories
link_directories(${PNG_LIB_DIR})

# Flags
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O3 -msse4.2")
set(CMAKE_BUILD_TYPE Release)

# Sources
file(GLOB SGMSTEREO_SRC_FILES "src/*.cpp")

add_executable(sgmstereo ${SGMSTEREO_SRC_FILES})
target_link_libraries(sgmstereo png)
