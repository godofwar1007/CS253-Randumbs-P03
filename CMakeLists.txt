cmake_minimum_required(VERSION 3.16)
project(dsa_churn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Sanitizer build (g++ or clang++ on Linux or WSL): cmake -DCHURN_SANITIZE=ON
option(CHURN_SANITIZE "Build with sanitizers" OFF)

add_library(churn_common INTERFACE)
target_include_directories(churn_common INTERFACE ${CMAKE_SOURCE_DIR}/include)

if(MSVC)
  target_compile_options(churn_common INTERFACE /W4)
  if(CHURN_SANITIZE)
    target_compile_options(churn_common INTERFACE /fsanitize=address /Zi)
  endif()
else()
  target_compile_options(churn_common INTERFACE -Wall -Wextra -Wpedantic)
  if(CHURN_SANITIZE)
    target_compile_options(churn_common INTERFACE
      -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1)
    target_link_options(churn_common INTERFACE -fsanitize=address,undefined)
  else()
    target_compile_options(churn_common INTERFACE -O2)
  endif()
endif()

add_executable(simulator simulator/simulator.cpp)
target_link_libraries(simulator PRIVATE churn_common)