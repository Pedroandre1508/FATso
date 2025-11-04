cmake_minimum_required(VERSION 3.12)
project(fat16tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fat16 STATIC
    src/fat16_image.cpp
    src/utils.cpp
)

target_include_directories(fat16 PUBLIC include)

add_executable(fat16tool src/main.cpp)

target_link_libraries(fat16tool PRIVATE fat16)

# Enable warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fat16 PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
  target_compile_options(fat16tool PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
