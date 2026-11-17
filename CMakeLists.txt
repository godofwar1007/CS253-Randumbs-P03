cmake_minimum_required(VERSION 3.16)
project(dsa_churn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

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

add_library(churn_scapegoat STATIC src/structB/scapegoat.cpp)
target_include_directories(churn_scapegoat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/structB)
target_link_libraries(churn_scapegoat PUBLIC churn_common)

add_library(churn_hibbard STATIC src/structA/hibbard.cpp)
target_include_directories(churn_hibbard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/structA)
target_link_libraries(churn_hibbard PUBLIC churn_common)

add_executable(simulator app/simulator.cpp)
target_link_libraries(simulator PRIVATE churn_hibbard churn_scapegoat)

add_executable(d6_pred docs/analysis/pred.cpp)
add_executable(d6_run bench/d6_run.cpp)
add_executable(d6_figures results/d6_figures.cpp)
foreach(target IN ITEMS d6_pred d6_run d6_figures)
  target_link_libraries(${target} PRIVATE churn_common)
endforeach()

add_executable(schedule_test adversary/schedule_test.cpp)
target_link_libraries(schedule_test PRIVATE churn_common)

add_executable(run_workload adversary/run_workload.cpp)
target_link_libraries(run_workload PRIVATE churn_hibbard churn_scapegoat)

add_executable(differential oracle/differential.cpp)
target_link_libraries(differential PRIVATE churn_hibbard churn_scapegoat)

add_executable(harness_test tests/harness_test.cpp)
target_link_libraries(harness_test PRIVATE churn_common)
target_include_directories(harness_test PRIVATE ${CMAKE_SOURCE_DIR}/oracle)

add_executable(scapegoat_tests tests/scapegoat_tests.cpp)
target_link_libraries(scapegoat_tests PRIVATE churn_scapegoat)

add_executable(oracle_checks tests/oracle_checks.cpp)
target_link_libraries(oracle_checks PRIVATE churn_hibbard churn_scapegoat)
target_include_directories(oracle_checks PRIVATE ${CMAKE_SOURCE_DIR}/oracle)

add_executable(adversarial_test tests/adversarial_test.cpp)
target_link_libraries(adversarial_test PRIVATE churn_common)
target_include_directories(adversarial_test PRIVATE ${CMAKE_SOURCE_DIR}/oracle)

enable_testing()
add_test(NAME adversarial_schedules COMMAND schedule_test)
add_test(NAME harness_test COMMAND harness_test)
add_test(NAME differential_all COMMAND differential --tree all --operations 1000000 --seed 20260928)
add_test(NAME scapegoat_tests COMMAND scapegoat_tests)
add_test(NAME oracle_checks COMMAND oracle_checks)
add_test(NAME differential_replay COMMAND differential --replay ${CMAKE_SOURCE_DIR}/tests/oracle_cases/deletions.txt)
add_test(NAME adversarial_generator COMMAND adversarial_test)
add_test(NAME differential_adversarial COMMAND differential --tree all --workload adversarial --size 128 --operations 1000000 --seed 20260928)
add_test(NAME adversarial_export COMMAND differential --tree all --workload adversarial --size 16 --operations 1024 --seed 42 --save ${CMAKE_CURRENT_BINARY_DIR}/adversarial_replay.txt)
set_tests_properties(adversarial_export PROPERTIES FIXTURES_SETUP adversarial_file)
add_test(NAME adversarial_replay COMMAND differential --replay ${CMAKE_CURRENT_BINARY_DIR}/adversarial_replay.txt)
set_tests_properties(adversarial_replay PROPERTIES FIXTURES_REQUIRED adversarial_file)
