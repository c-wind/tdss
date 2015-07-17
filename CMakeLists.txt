PROJECT(tdss)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

MESSAGE(STATUS "Project: TDSS")
MESSAGE(STATUS "Project Directory: ${PROJECT_SOURCE_DIR}")

# include directories
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/src/lib_mrt
  ${PROJECT_SOURCE_DIR}/src/lib_tdss

  /usr/local/include
  /usr/include
)

# lib directories
LINK_DIRECTORIES(
  ${PROJECT_BINARY_DIR}/src/lib_mrt
  ${PROJECT_BINARY_DIR}/src/lib_tdss

  /usr/local/lib
  /use/lib
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_SUBDIRECTORY(src/lib_mrt)
ADD_SUBDIRECTORY(src/lib_tdss)
ADD_SUBDIRECTORY(src/master_server)
ADD_SUBDIRECTORY(src/data_server)
ADD_SUBDIRECTORY(src/name_server)
