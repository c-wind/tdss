#include <openssl/conf.h>
#include "global.h"
#include "tdss_config.h"


data_server_conf_t      ds_conf = {0};
name_server_conf_t      ns_conf = {0};
master_server_conf_t    ms_conf = {0};



int data_server_config_load(char *fname)
{
    __label__ error_return;
    CONF *conf = NULL;
    long ret = 0;
    char *pstr = NULL, sn[64] = {0}, data_path[MAX_PATH] = {0}; //sn=server name
    int sc = 0, i = 0, sid = 0, max_block_size = 0, log_level = 1, timeout = 5; //sc=server count,sid=server id
    data_server_t *ds = NULL;

    conf=NCONF_new(NULL);

    if(!NCONF_load(conf, fname, &ret))
    {
        log_error("open conf:%s error line:%ld.", fname, ret);
        goto error_return;
    }

    if(!(pstr = NCONF_get_string(conf, "global", "server_count")))
    {
        log_error("conf no found global server_count");
        goto error_return;
    }
    sc = atoi(pstr);
    if(sc < 1)
    {
        log_error("data server count < 1");
        goto error_return;
    }

    ds = M_alloc(sizeof(data_server_t) * sc);
    if(!ds)
    {
        log_error("M_alloc data_server error:%m");
        goto error_return;
    }

    if(!(pstr = NCONF_get_string(conf, "global", "max_block_size")))
    {
        log_error("conf no found global max_block_size");
        goto error_return;
    }
    max_block_size = atoi(pstr);

    if((pstr = NCONF_get_string(conf, "global", "log_level")))
    {
        log_level = atoi(pstr);
    }

    if((pstr = NCONF_get_string(conf, "global", "timeout")))
    {
        timeout = atoi(pstr);
    }

    if(!(pstr = NCONF_get_string(conf, "global", "data_path")))
    {
        log_error("conf no found global data_path");
        goto error_return;
    }
    snprintf(data_path, sizeof(data_path) - 1, "%s", pstr);


    for(i=0; i< sc; i++)
    {
        snprintf(sn, sizeof(sn) - 1, "server_%d", i+1);
        if(!(pstr = NCONF_get_string(conf, sn, "addr")))
        {
            log_error("conf no found %s addr", sn);
            goto error_return;
        }

        if(sscanf(pstr, "%[^:]:%d", ds[i].server.ip, &ds[i].server.port) != 2)
        {
            log_error("conf %s addr:(%s) error", sn, pstr);
            goto error_return;
        }

        if(!memcmp(&ds[i].server, &ds_conf.local, sizeof(ds[i].server)))
        {
            log_debug("local:(%s:%d) current server[%d]:(%s:%d)",
                      ds_conf.local.ip, ds_conf.local.port,
                      i, ds[i].server.ip, ds[i].server.port);
            sid = i;
        }
    }

    if(ds_conf.server)
    {
        M_free(ds_conf.server);
    }
    ds_conf.server_count = sc;
    ds_conf.server = ds;
    ds_conf.server_id = sid;
    ds_conf.max_block_size = max_block_size;
    ds_conf.log_level = log_level;
    ds_conf.timeout = timeout;
    snprintf(ds_conf.data_path, sizeof(ds_conf.data_path) -1, "%s",  data_path);

    NCONF_free(conf);

    return MRT_SUC;

error_return:
    if(conf)
        NCONF_free(conf);
    if(ds)
    {
        M_free(ds);
    }

    return MRT_ERR;
}





/* [global]
 * server_count = 2
 * [server_1]
 * master=127.0.0.1:8010
 * slave=127.0.0.1:8011
 * [server_2]
 * master=127.0.0.1:8020
 * slave=127.0.0.1:8021
 *
 */

int ns_get_master_addr(char *key, int klen, char **ip, int *port)
{
    uint32_t idx = 0;

    M_cpvril(key);

    idx = crc32(0L, NULL, 0);

    idx = crc32(idx, key, klen);

    idx &= (ns_conf.server_count - 1);

    log_debug("idx:%d", idx);

    *ip = ns_conf.server[idx].master.ip;
    *port = ns_conf.server[idx].master.port;

    return MRT_SUC;
}



int ns_get_slave_addr(char *key, int klen, char **ip, int *port)
{
    uint32_t idx = 0;

    idx = crc32(0L, NULL, 0);

    idx = crc32(idx, key, klen);

    idx &= (ns_conf.server_count - 1);

    log_debug("idx:%d", idx);

    *ip = ns_conf.server[idx].slave.ip;
    *port = ns_conf.server[idx].slave.port;

    return MRT_SUC;
}

int ds_get_addr_by_id(int server_id, char **ip, int *port)
{
    if(ds_conf.server_count < server_id || server_id < 0)
        return MRT_ERR;

    *ip = ds_conf.server[server_id].server.ip;
    *port = ds_conf.server[server_id].server.port;

    return MRT_OK;
}


int ns_get_server_id(char *mid, int *id)
{
    uint64_t idx = 0;

    if(sscanf(mid, "%jx", &idx) != 1)
    {
        log_error("mid:(%s) error.", mid);
        return MRT_ERR;
    }

    *id = idx & (ns_conf.server_count - 1);

    return MRT_SUC;
}





int name_server_config_load(char *fname)
{
    __label__ error_return;
    CONF *conf = NULL;
    long ret = 0;
    char *pstr = NULL, sn[64] = {0};    //sn=server name
    int sc = 0, i = 0, sid = 0, st = 0, log_level = 1, timeout = 5; //sc=server count sid=server id st=server status
    name_server_t *ns = NULL;

    conf=NCONF_new(NULL);

    if(!NCONF_load(conf, fname, &ret))
    {
        log_error("open conf:%s error line:%ld.", fname, ret);
        goto error_return;
    }

    if(!(pstr = NCONF_get_string(conf, "global", "server_count")))
    {
        log_error("conf no found global server_count");
        goto error_return;
    }

    sc = atoi(pstr);
    if(sc < 1)
    {
        log_error("name server count < 1");
        goto error_return;
    }

    ns = M_alloc(sizeof(name_server_t) * sc);
    if(!ns)
    {
        log_error("M_alloc name_server error:%m");
        goto error_return;
    }

    if((pstr = NCONF_get_string(conf, "global", "log_level")))
    {
        log_level = atoi(pstr);
    }

    if((pstr = NCONF_get_string(conf, "global", "timeout")))
    {
        timeout = atoi(pstr);
    }

    for(i=0; i< sc; i++)
    {
        snprintf(sn, sizeof(sn) - 1, "server_%d", i+1);
        if(!(pstr = NCONF_get_string(conf, sn, "master")))
        {
            log_error("conf no found %s master", sn);
            goto error_return;
        }

        if(sscanf(pstr, "%[^:]:%d", ns[i].master.ip, &ns[i].master.port) != 2)
        {
            log_error("conf %s master error", sn);
            goto error_return;
        }

        if(!(pstr = NCONF_get_string(conf, sn, "slave")))
        {
            log_error("conf no found %s slave", sn);
            goto error_return;
        }

        if(sscanf(pstr, "%[^:]:%d", ns[i].slave.ip, &ns[i].slave.port) != 2)
        {
            log_error("conf %s slave error", sn);
            goto error_return;
        }

        if(!memcmp(&ns[i].master, &ns_conf.local, sizeof(ns[i].master)))
        {
            log_debug("local:(%s:%d) current server[%d]->master:(%s:%d)",
                      ns_conf.local.ip, ns_conf.local.port,
                      i, ns[i].master.ip, ns[i].master.port);
            sid = i;
            st = 1;
            continue;
        }

        if(!memcmp(&ns[i].slave, &ns_conf.local, sizeof(ns[i].slave)))
        {
            log_debug("local:(%s:%d) current server[%d]->slave:(%s:%d)",
                      ns_conf.local.ip, ns_conf.local.port,
                      i, ns[i].slave.ip, ns[i].slave.port);
            sid = i;
            st = 2;
        }
    }

    if(!(pstr = NCONF_get_string(conf, "file_info", "db_path")))
    {
        log_error("conf no found file_info db_path");
        goto error_return;
    }
    snprintf(ns_conf.db_path, sizeof(ns_conf.db_path), "%s", pstr);

    if(ns_conf.server)
    {
        M_free(ns_conf.server);
    }
    ns_conf.server_count = sc;
    ns_conf.server = ns;
    ns_conf.server_type = st;
    ns_conf.server_id = sid;
    ns_conf.log_level = log_level;
    ns_conf.timeout = timeout;

    NCONF_free(conf);

    return MRT_SUC;

error_return:
    if(conf)
        NCONF_free(conf);
    if(ns)
        M_free(ns);

    return MRT_ERR;
}


int master_server_config_load(char *fname)
{
    __label__ error_return;
    CONF *conf = NULL;
    long ret = 0;
    char *pstr = NULL;

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
    ms_conf.log_level = atoi(pstr);

    if(!(pstr = NCONF_get_string(conf, "global", "timeout")))
    {
        log_error("conf no found global timeout");
        goto error_return;
    }
    ms_conf.timeout = atoi(pstr);

    if(!(pstr = NCONF_get_string(conf, "global", "interval")))
    {
        log_error("conf no found global interval");
        goto error_return;
    }
    ms_conf.interval = atoi(pstr);

    NCONF_free(conf);

    return MRT_SUC;

error_return:
    if(conf)
        NCONF_free(conf);

    return MRT_ERR;
}

