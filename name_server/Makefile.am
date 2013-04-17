INCLUDES= -I../lib_mrt/ -I../lib_tdss/
bin_PROGRAMS=name_server
name_server_SOURCES= command.c  fblock_mem.c  name_server.c  name_server_sync.c  process.c
name_server_LDADD=../lib_tdss/lib_tdss.a ../lib_mrt/lib_mrt.a -lssl
AUTOMAKE_OPTIONS=foreign
