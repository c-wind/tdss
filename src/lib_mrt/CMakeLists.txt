SET(LIBMRT_SRC base64.c binmap.c buffer.c charset.c comm_func.c event_center.c factory.c file_buffer.c file_func.c hashdb.c hashmap.c ini_func.c logger.c mem_pool.c socket_func.c string.c strutil.c)

ADD_LIBRARY(mrt STATIC ${LIBMRT_SRC})

INSTALL(TARGETS mrt ARCHIVE DESTINATION lib)

INSTALL(FILES binmap.h buffer.h charset_func.h comm_func.h encrypt_func.h event_center.h factory.h file_func.h global.h hashdb.h hashmap.h http_func.h ini_func.h list_func.h logger.h macro_const.h macro_func.h mem_pool.h minheap.h mysql_func.h socket_func.h string_func.h DESTINATION include/libmrt)

