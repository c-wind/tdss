SET(SOURCE_FILES command.c master.c process.c server_check.c)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE_DIRECTORIES( 
      /usr/local/include
      ${PROJECT_SOURCE_DIR}/src/lib_mrt
      ${PROJECT_SOURCE_DIR}/src/lib_tdss
     )

#LINK_LIBRARIES(${PROJECT_SOURCE_DIR}/lib)

ADD_EXECUTABLE(master_server ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(master_server pthread)
TARGET_LINK_LIBRARIES(master_server crypto)
TARGET_LINK_LIBRARIES(master_server z)
TARGET_LINK_LIBRARIES(master_server mrt)
TARGET_LINK_LIBRARIES(master_server tdss)

SET_TARGET_PROPERTIES(master_server  PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin") 

