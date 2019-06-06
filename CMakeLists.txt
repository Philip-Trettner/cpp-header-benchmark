cmake_minimum_required(VERSION 3.8)
project(cpp-header-benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.*")

add_executable(${PROJECT_NAME} ${SOURCES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC 
        /MP 
        /FC
    )
else()
    target_compile_options(${PROJECT_NAME} PUBLIC 
        -Wall
    )
endif()
