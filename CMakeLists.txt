PROJECT(new-eden-route-derivation)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# General compiler flags.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers ")

# Set some g++ specific options.
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g")
ENDIF()

# Check if OpenMP is available so we can do multithreading.
FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()

# Apparently we do some C++11 things so we require that standard.
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(include)

# The first part of the entire process is creating the eve_nerd library.
ADD_LIBRARY(eve_nerd SHARED src/dijkstra.cpp src/min_heap.cpp src/universe.cpp)

# The executable is just an extra pretty much.
ADD_EXECUTABLE(eve_nerd_bin src/main.cpp)
SET_TARGET_PROPERTIES(eve_nerd_bin PROPERTIES OUTPUT_NAME eve_nerd)
TARGET_LINK_LIBRARIES(eve_nerd_bin readline eve_nerd)

# This part does SWIG things to make a Python library.
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

SET_SOURCE_FILES_PROPERTIES(eve_nerd.i PROPERTIES CPLUSPLUS ON)
SWIG_ADD_MODULE(eve_nerd python eve_nerd.i)
SWIG_LINK_LIBRARIES(eve_nerd eve_nerd)
