#include "global.h"

factory_t factory;

#define factory_lock() pthread_mutex_lock(&factory.worker_mutex)
#define factory_unlock() pthread_mutex_unlock(&factory.worker_mutex)

#define worker_wakeup(wkr) pthread_cond_signal(&(wkr->cnd))

#define master_wakeup() \
    do \
{ \
    pthread_mutex_lock(&(factory.master.mtx)); \
    pthread_cond_signal(&(factory.master.cnd)); \
    pthread_mutex_unlock(&(factory.master.mtx)); \
}while(0);



int worker_wait(worker_t *wkr)
{
    M_cpvril(wkr);

    M_ciril(pthread_mutex_lock(&(wkr->mtx)), "lock worker's mutex error.");

    M_ciril(pthread_cond_wait(&(wkr->cnd), &(wkr->mtx)), "wait worker's cond error.");

    pthread_mutex_unlock(&(wkr->mtx));

    return MRT_SUC;
}







void *worker_init(void *arg)
{
    worker_t *wkr = NULL;
    int iret = 0;

    M_cpvrvl(arg);

    wkr = (worker_t *) arg;

    pthread_detach(pthread_self());
    pthread_setspecific(factory.key, wkr);

    wkr->idx = pthread_self();

    //等待启动信号
    M_cirvnl(worker_wait(wkr));

    wkr->state = WORKER_START;

    log_info("worker:(%lu) start, will run %s.",
             wkr->idx, wkr->proc.name);

    while(factory.state != FACTORY_OVER)
    {
        if(!wkr->proc.argv)
            iret = wkr->proc.func(wkr);
        else
            iret = wkr->proc.func(wkr->proc.argv);

        if(iret == MRT_ERR)
        {
            log_error("run %s return:%d.", wkr->proc.name, iret);
            return NULL;
        }
        log_info("run %s return:%d.", wkr->proc.name, iret);
    }

    return NULL;
}

void *worker_deinit(void *arg)
{
    worker_t *wkr = (worker_t *) arg;

    M_cirvl(factory_lock(), "lock factory error, %m");
    M_list_remove(&factory, wkr);
    //if(factory.max_run_times)
    {
        //factory.master.func = wkr->func;
        M_free(wkr);
        master_wakeup();
    }
    factory.conf.worker_num--;
    factory_unlock();

    return NULL;
}



int worker_create(callback_t cb)
{
    worker_t *wkr = NULL;

    M_ciril(factory_lock(), "lock factory error:%m");
    M_cvril((wkr = (worker_t *)M_alloc(sizeof(worker_t))), "malloc worker error, %m.");

    pthread_mutex_init(&(wkr->mtx), NULL);
    pthread_cond_init(&(wkr->cnd), NULL);

    wkr->state = WORKER_READY;
    wkr->proc = cb;

    if(pthread_create(&(wkr->idx), NULL, worker_init, (void *)wkr) != 0)
    {
        log_error("pthread_create error.%m");
        return MRT_ERR;
    }

    M_list_insert_head(&factory, wkr);

    factory.conf.worker_num++;

    M_ciril(factory_unlock(), "unlock factory error, %m");

    while(wkr->state == WORKER_READY)
    {
        worker_wakeup(wkr);
        usleep(100);
        continue;
    }

    log_debug("create success, worker:(%lu), worker num:%x", wkr->idx, factory.conf.worker_num);

    return MRT_SUC;
}

void *worker_master(void *arg)
{
    worker_t *wkr = NULL;
    callback_t cb;

    M_cpvrvl(arg);

    wkr = (worker_t *)arg;

    pthread_detach(pthread_self());
    pthread_setspecific(factory.key, wkr);

    wkr->idx = pthread_self();
    wkr->state = WORKER_START;

    callback_set(cb, process_center_loop, NULL);

    while(1)
    {
        log_info("process center check...");
        if(process_center_check() == 1)
        {
            if(worker_create(cb) == MRT_ERR)
                log_error("create process center worker error.");
        }
        sleep(5);
    }

    return NULL;
}



int factory_init(server_config_t sconf, callback_t proc)
{
    s_zero(factory);

    M_list_init(&factory);

    pthread_mutex_init(&factory.worker_mutex, NULL);
    pthread_key_create(&(factory.key), (void *)&worker_deinit);
    pthread_setspecific(factory.key, &(factory.master));

    if(sconf.logger == 1)
    {
        if(logger_init("./log/", sconf.logger_name, sconf.logger_level))
            return MRT_ERR;
    }

    //最大线程数，默认为1
    factory.conf.worker_max = sconf.worker_max > 0 ? sconf.worker_max : 1;
    //最小线程数，默认为1
    factory.conf.worker_min = sconf.worker_min > 0 ? sconf.worker_min : 1;

    if(sconf.local_bind == 1)
    {
        //绑定IP，默认为0.0.0.0
        if(!*sconf.local_host)
            snprintf(factory.conf.local_host, sizeof(factory.conf.local_host), "0.0.0.0");
        else
            snprintf(factory.conf.local_host, sizeof(factory.conf.local_host), "%s", sconf.local_host);
        //绑定端口，默认为2345
        factory.conf.local_port = sconf.local_port > 1 ? sconf.local_port : 2345;
        //最大连接数，默认为1024
        factory.conf.conn_max       = sconf.conn_max > M_1KB ? sconf.conn_max : M_1KB;
        factory.conf.conn_timeout   = sconf.conn_timeout > 1 ? sconf.conn_timeout : 5;

        factory.conf.local_bind = 1;

        /*
        M_ciril(event_center_init(&factory.event_center, factory.conf.conn_max, factory.conf.conn_timeout,
                                  factory.conf.local_host,
                                  factory.conf.local_port,
                                  ev_init, ev_deinit),
                "init event center error.");
                */


    }

    if(proc.state == 1)
    {
        M_ciril(process_center_init(factory.conf.worker_max,
                                    factory.conf.worker_min,
                                    proc),
                "init process center error.");
    }

    if(sconf.daemon == 1)
    {
        //如果daemon_home有值就切换到daemon_home目录，否则以当前目录为中心
        if(daemon_init(*sconf.daemon_home ? sconf.daemon_home : "./"))
        {
            printf("daemon init error:%m\n");
            return MRT_ERR;
        }
    }

    log_info("factory init success.");

    factory.state = FACTORY_READY;

    return MRT_SUC;
}


int factory_start(int backend, int type)
{
    worker_t *wkr = NULL;
    int i=0;
    callback_t cb;

    if(type == FACTORY_SINGLE)
    {
        factory.event_center.worker_num = 1;
        callback_set(cb, event_single_loop, &factory.event_center);
        M_ciril(worker_create(cb), "create event single loop error.");
    }
    else
    {
        callback_set(cb, process_center_loop, NULL);
        for(i=0; i< factory.conf.worker_min; i++)
        {
            M_ciril(worker_create(cb),
                    "create process center worker %d error.", i);
        }
    }
    factory.state = FACTORY_START;

    log_info("factory start success.");

    if(backend)
    {
        callback_set(cb, worker_master, NULL);
        M_ciril(worker_create(cb), "create process center worker error.");
    }
    else
    {
        wkr = &factory.master;
        if(worker_master(wkr))
        {
            log_error("worker master return error.");
            return -1;
        }
    }

    return MRT_SUC;
}



int factory_deinit()
{
    M_ciril(factory_lock(), "lock factory error, %m");

    log_info("factory will stop, There are %d workers are working.",
             factory.conf.worker_num);

    factory.state = FACTORY_OVER;

    factory_unlock();

    while(factory.conf.worker_num > 0)
    {
        log_info("wait for worker exit, current has %d.", factory.conf.worker_num);
        sleep(1);
    }

    log_info("current worker num:%d, factory stop!", factory.conf.worker_num);

    return MRT_OK;
}






#ifdef FACTORY_TEST

int test(worker_t *wkr)
{
    //    printf("wkr:%u in %s.\n", wkr->idx, __func__);

    char *pstr = M_alloc(125);
    sprintf(pstr, "%lu", worker_id);
    printf("pstr:%s\n", pstr);

    sleep(1);

    //   printf("wkr:%u out %s.\n", wkr->idx, __func__);

    return 0;
}

int main()
{

    log_init("./", "factory", "debug");
    factory_init(5);

    worker_create(test);

    worker_create(test);

    sleep(100);

    factory_destory();

    return 0;
}

#endif


