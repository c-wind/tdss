INCLUDES= -I../lib_mrt/ -I../lib_tdss/
bin_PROGRAMS=tdss_client
tdss_client_SOURCES=tdss_client.c tdss_config.c
tdss_client_LDADD= ../lib_tdss/lib_tdss.a ../lib_mrt/lib_mrt.a -lssl
AUTOMAKE_OPTIONS=foreign
