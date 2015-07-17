SET(SOURCE_FILES command.c fblock_mem.c name_server.c name_server_sync.c process.c)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 

INCLUDE_DIRECTORIES( 
      /usr/local/include
      ${PROJECT_SOURCE_DIR}/src/lib_mrt
      ${PROJECT_SOURCE_DIR}/src/lib_tdss
     )

ADD_EXECUTABLE(name_server  ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(name_server pthread)
TARGET_LINK_LIBRARIES(name_server crypto)
TARGET_LINK_LIBRARIES(name_server z)
TARGET_LINK_LIBRARIES(name_server mrt)
TARGET_LINK_LIBRARIES(name_server tdss)

SET_TARGET_PROPERTIES(name_server  PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin") 

