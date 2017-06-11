PROJECT(new-eden-route-derivation)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -O3")
ENDIF()

FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()

ADD_LIBRARY(eve_nerd SHARED src/dijkstra.cpp src/min_heap.cpp src/universe.cpp)
ADD_EXECUTABLE(eve_nerd_bin src/main.cpp)

INCLUDE_DIRECTORIES(include)

SET_TARGET_PROPERTIES(eve_nerd_bin PROPERTIES OUTPUT_NAME eve_nerd)

TARGET_LINK_LIBRARIES(eve_nerd_bin readline)
TARGET_LINK_LIBRARIES(eve_nerd_bin eve_nerd)
