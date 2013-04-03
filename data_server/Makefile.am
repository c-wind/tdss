INCLUDES= -I../lib_mrt/ -I../lib_tdss/
bin_PROGRAMS=data_server
data_server_SOURCES=command.c  data_file.c  data_server.c  name_server_api.c  process.c
data_server_LDADD=../lib_mrt/lib_mrt.a ../lib_tdss/lib_tdss.a -lssl
AUTOMAKE_OPTIONS=foreign
