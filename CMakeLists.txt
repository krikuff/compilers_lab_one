cmake_minimum_required(VERSION 3.19.7)

project(lab1c)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -Werror)

include_directories(${CMAKE_SOURCE_DIR})

set(HEADERS
    compilation.h
    errors.h
    helpers.h
    pda.h
    )
set(SOURCES
    compilation.cpp
    helpers.cpp
    main.cpp
    )

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
