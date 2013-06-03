INCLUDES= -I../lib_mrt/ -I../lib_tdss/
bin_PROGRAMS=master_server
master_server_SOURCES=command.c  master.c  process.c  server_check.c
master_server_LDADD= ../lib_tdss/lib_tdss.a ../lib_mrt/lib_mrt.a -lssl
AUTOMAKE_OPTIONS=foreign
