#include "global.h"

int ini_file_load(char *file_name, hashmap_t *hmp)
{
    FILE *fp = NULL;
    char ckey[SHORT_STR] = {0};
    char line_buf[MAX_LINE] = {0};
    char lable[SHORT_STR] = {0};
    char key[SHORT_STR] = {0};
    char val[SHORT_STR] = {0};

    M_ciril(hashmap_init(hmp, 1024), "init hashmap error");

    M_cvril((fp = fopen(file_name, "r")),
            "fopen error:%m, file:%s.", file_name);

    while(fgets(line_buf, sizeof(line_buf), fp))
    {
        if(note_filter(line_buf))
        {
            p_zero(line_buf);
            continue;
        }

        if(*line_buf == '[' &&
           line_buf[strlen(line_buf) - 1] == ']' &&
           strlen(line_buf) > 2)
        {
            p_zero(lable);
            snprintf(lable, strlen(line_buf) - 1, "%s", line_buf+1);
            continue;
        }

        if((sscanf(line_buf, "%[^=]=%s", key, val) == 2) && (!str_format(key) && !str_format(val)))
        {
            p_zero(ckey);
            sprintf(ckey, "%s|%s", lable, key);
            M_cvril(hashmap_insert(hmp, str_newcpy(ckey, strlen(ckey)), strlen(ckey), str_newcpy(val, strlen(val))),
                    "insert hash map error, key:%s, val:%s", ckey, val);
        }

        p_zero(line_buf);
        p_zero(key);
        p_zero(val);
    }

    fclose(fp);

    return MRT_SUC;
}


static void key_free(void *key)
{
    M_free(key);
}

static void val_free(void *val)
{
    M_free(val);
}

void ini_file_unload(hashmap_t *hmp)
{
    hashmap_free(hmp, key_free, val_free);
}



int ini_get_int(hashmap_t *hmp, char *part, char *key, int def)
{
    char *var = NULL;
    char nkey[64] = {0};

    M_cpsril(key);
    M_cpsril(part);

    sprintf(nkey, "%s|%s", part, key);

    if(!(var = hashmap_find(hmp, nkey, strlen(nkey))))
    {
        log_error("no found key:%s", nkey);
        return def;
    }

    return  atoi(var);
}


char *ini_get_str(hashmap_t *hmp, char *part, char *key, char *def)
{
    char *var = NULL;
    char nkey[64] = {0};

    M_cpsrvl(key);
    M_cpsrvl(part);

    sprintf(nkey, "%s|%s", part, key);

    if(!(var = hashmap_find(hmp, nkey, strlen(nkey))))
    {
        log_error("no found key:%s", nkey);
        return def;
    }

    return  (var);
}





#ifdef CONF_TEST

int main()
{
    memory_pool_init();
    hashmap_t *hmp = NULL;

    if(load_ini_file("./test.conf", &hmp) == MRT_ERR)
    {
        printf("load ini error.\n");
        return -1;
    }

    printf("abc=%s\n", hashmap_find(hmp, "main|abc", strlen("main|abc")));
    printf("bcd=%s\n", hashmap_find(hmp, "main|bcd", strlen("main|bcd")));
    printf("ddd=%s\n", hashmap_find(hmp, "mmtt|ddd", strlen("mmtt|ddd")));


    return 0;
}

#endif


