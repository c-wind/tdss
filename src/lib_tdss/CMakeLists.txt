SET(LIBMRT_SRC config.c inet_event.c request.c time_queue.c)

ADD_LIBRARY(tdss STATIC ${LIBMRT_SRC})

INSTALL(TARGETS tdss ARCHIVE DESTINATION lib)

INSTALL(FILES inet_event.h tdss_config.h time_queue.h  DESTINATION include/libtdss)
