cmake_minimum_required(VERSION 3.0.0)
project(ANNA)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_DEBUG_POSTFIX d)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -lomp")

add_subdirectory(./dramsim3)

file(GLOB_RECURSE multi_anna_src "multi_anna/*.cc" "multi_anna/*.cpp" "multi_anna/*.c" "multi_anna/*.hpp")
add_library(MultiANNA STATIC ${multi_anna_src})
target_link_libraries(MultiANNA PRIVATE args dramsim3)
target_include_directories(MultiANNA PRIVATE multi_anna dramsim3/src dramsim3/ext)

add_executable(multiannatest multi_anna/test.cpp)
target_include_directories(multiannatest PRIVATE multi_anna dramsim3/src dramsim3/ext/)
target_link_libraries(multiannatest PRIVATE dramsim3 MultiANNA args)
target_compile_options(multiannatest PRIVATE)
set_target_properties(multiannatest PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )
