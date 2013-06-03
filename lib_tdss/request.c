#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"


int request_parse(string_t *src, rq_arg_t *arg, int asize)
{
    int i = 0, c, state = 0, num = 0, idx = 0, k = 0;
    char key[128] = { 0 }, val[1024] = { 0};

    for(k=0; k<src->len; k++)
    {
        c = src->str[k];
        switch (c)
        {
        case '=':
            state = 1;
            idx = 0;
            break;
        case '\r':
        case '\n':
        case ' ':
            state = 2;
            idx = 0;
            break;
        default:
            if (state == 0)
            {
                if(idx < sizeof(key))
                    key[idx++] = c;
            }
            else
            {
                if(idx < sizeof(val))
                    val[idx++] = c;
            }
            break;
        }
        if (state == 2)
        {
            if (strlen(key) > 0 && strlen(val) > 0)
            {
                for(i=0; i<asize; i++)
                {
                    if (!strcasecmp(arg[i].key, key))
                    {
                        switch(arg[i].type)
                        {
                        case RQ_TYPE_U8:
                            *(uint8_t *)arg[i].val = (uint8_t)atoi(val);
                            break;
                        case RQ_TYPE_U16:
                            *(uint16_t *)arg[i].val = (uint16_t)atoi(val);
                            break;
                        case RQ_TYPE_U32:
                            *(uint32_t *)arg[i].val = (uint32_t)atoi(val);
                            break;
                        case RQ_TYPE_U64:
                            *(uint64_t *)arg[i].val = (uint64_t)atoll(val);
                            break;
                        case RQ_TYPE_STR:
                            snprintf((char *)arg[i].val, arg[i].vsize, "%s", val);
                            break;
                        }
                        num++;
                        break;
                    }
                }

                if (num == asize)
                {
                    log_debug("find argc=2");
                    return 0;
                }
            }
            memset(key, 0, sizeof(key));
            memset(val, 0, sizeof(val));
            state = 0;
        }
    }
    log_error("find argc:%d", num);
    return -1;
}

