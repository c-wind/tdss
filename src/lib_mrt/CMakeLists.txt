SET(LIBMRT_SRC base64.c buffer.c comm_func.c  factory.c  file_func.c hashmap.c logger.c mem_pool.c mysql_func.c  socket_func.c strutil.c binmap.c charset.c event_center.c file_buffer.c hashdb.c  ini_func.c mem_line.c minheap.c process_center.c string.c  xxtea.c)

ADD_LIBRARY(mrt STATIC ${LIBMRT_SRC})

INSTALL(TARGETS mrt ARCHIVE DESTINATION lib)

INSTALL(FILES binmap.h  charset_func.h  encrypt_func.h  file_func.h  ftplib.h  hashdb.h   http_func.h  list_func.h  macro_const.h  mem_pool.h  mysql_func.h   string_func.h buffer.h  comm_func.h     factory.h       ftp_func.h   global.h  hashmap.h  ini_func.h   logger.h     macro_func.h   minheap.h   socket_func.h DESTINATION include/libmrt)

