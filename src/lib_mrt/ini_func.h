#ifndef __INI_FUNC_H__
#define __INI_FUNC_H__




// ----------- 函数声明 ----------------

int ini_file_load(char *file_name, hashmap_t *hmp);

void ini_file_unload(hashmap_t *hmp);

int ini_get_int(hashmap_t *hmp, char *, char *key, int def);

char *ini_get_str(hashmap_t *hmp, char *, char *key, char *def);


// ------------------------------------


#endif
