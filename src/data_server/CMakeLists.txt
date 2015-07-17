SET(SOURCE_FILES command.c data_file.c data_server.c name_server_api.c process.c)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE_DIRECTORIES( 
      /usr/local/include
      ${PROJECT_SOURCE_DIR}/src/lib_mrt
      ${PROJECT_SOURCE_DIR}/src/lib_tdss
     )


ADD_EXECUTABLE(data_server  ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(data_server pthread)
TARGET_LINK_LIBRARIES(data_server crypto)
TARGET_LINK_LIBRARIES(data_server z)
TARGET_LINK_LIBRARIES(data_server mrt)
TARGET_LINK_LIBRARIES(data_server tdss)

SET_TARGET_PROPERTIES(data_server  PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin") 

