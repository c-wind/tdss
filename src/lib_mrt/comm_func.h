#ifndef __COMM_FUNC_H__
#define __COMM_FUNC_H__

// ----------- 函数声明 ----------------

int daemon_init(char *home);

/*
int trust_addr_array_init(T_array *ary, char *addrs, char *separator);

int trust_addr_check(T_array *ary, char *addr);
*/

void set_error(char *msg, ...);

char *get_error();


char *printip(unsigned char * ip);

char *int64toa(off_t num, char * buf, int con_unit);

char *get_port_fmt(int ip, unsigned int port);

void clear_path(char * path);

char *get_relative_path(char * src, char * dst);

char * unescape(char * str);

char *legible(off_t l);

int numdigit (long number);

//char *basename(char * p);

// -------------------------------------
//

#endif
