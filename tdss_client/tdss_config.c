#include <openssl/conf.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "tdss_client.h"


extern tdss_client_t tc;



int tdss_client_config_load(char *fname)
{
    __label__ error_return;
    CONF *conf = NULL;
    long ret = 0;
    char *pstr = NULL, sn[64] = {0}; //sn=server name

    conf=NCONF_new(NULL);

    if(!NCONF_load(conf, fname, &ret))
    {
        log_error("open conf:%s error line:%ld.", fname, ret);
        goto error_return;
    }

    if(!(pstr = NCONF_get_string(conf, "global", "log_level")))
    {
        log_error("conf no found global log_level");
        goto error_return;
    }
    tc.log_level = atoi(pstr);

    if(!(pstr = NCONF_get_string(conf, "global", "timeout")))
    {
        log_error("conf no found global timeout");
        goto error_return;
    }
    tc.timeout = atoi(pstr);

    if(!(pstr = NCONF_get_string(conf, "global", "master")))
    {
        log_error("conf no found %s addr", sn);
        goto error_return;
    }

    if(sscanf(pstr, "%[^:]:%d", tc.master.ip, &tc.master.port) != 2)
    {
        log_error("conf global master:(%s) error", pstr);
        goto error_return;
    }

    if(conf)
        NCONF_free(conf);

    return MRT_SUC;

error_return:
    if(conf)
        NCONF_free(conf);

    return MRT_ERR;
}

