#include "global.h"

factory_t factory;

#define worker_wakeup(wkr) pthread_cond_signal(&(wkr->cnd))

#define master_wakeup() \
    do \
{ \
    pthread_mutex_lock(&(factory.master.mtx)); \
    pthread_cond_signal(&(factory.master.cnd)); \
    pthread_mutex_unlock(&(factory.master.mtx)); \
}while(0);

#define factory_task_lock() pthread_mutex_lock(&factory.task_mtx)
#define factory_task_wakeup() pthread_cond_signal(&factory.task_cnd)
#define factory_task_wait() pthread_cond_wait(&factory.task_cnd, &factory.task_mtx)
#define factory_task_unlock() pthread_mutex_unlock(&factory.task_mtx)

#define factory_lock() pthread_mutex_lock(&factory.mtx)
#define factory_wakeup() pthread_cond_signal(&factory.cnd)
#define factory_wakeup_all() pthread_cond_broadcast(&factory.cnd)
#define factory_wait() pthread_cond_wait(&factory.cnd, &factory.mtx)
#define factory_unlock() pthread_mutex_unlock(&factory.mtx)

static void *worker_deinit(void *arg);

int factory_init(int wkr_max, int wkr_min)
{
    s_zero(factory);

    LIST_INIT(&factory, task_head);
    LIST_INIT(&factory, worker_head);

    factory.state = FACTORY_READY;

    pthread_mutex_init(&factory.mtx, NULL);
    pthread_cond_init(&factory.cnd, NULL);
    pthread_key_create(&factory.key, (void *)&worker_deinit);
    pthread_setspecific(factory.key, &(factory.master));

    factory.worker_max = wkr_max;
    factory.worker_min = wkr_min;


    return MRT_SUC;
}


void factory_task_push(task_t *tsk)
{
    factory_task_lock();

    LIST_INSERT_TAIL(&factory, task_head, tsk, node);

    factory.task_num++;

    factory_task_wakeup();

    factory_task_unlock();
}

int factory_task_pop(worker_t *wkr, task_t **ntsk)
{
    factory_task_lock();

    while(!factory.task_num || LIST_EMPTY(&factory, task_head))
    {
        log_debug("%lx wait task.", wkr->idx);
        factory_task_wait();
    }

    *ntsk = LIST_FIRST(&factory, task_head);

    LIST_REMOVE_HEAD(&factory, task_head);
    factory.task_num--;

    factory_task_unlock();

    return MRT_SUC;
}

int worker_wait(worker_t *wkr)
{
    M_cpvril(wkr);

    M_ciril(pthread_mutex_lock(&(wkr->mtx)), "lock worker's mutex error.");

    M_ciril(pthread_cond_wait(&(wkr->cnd), &(wkr->mtx)), "wait worker's cond error.");

    pthread_mutex_unlock(&(wkr->mtx));

    return MRT_SUC;
}


int factory_worker_count()
{
    int size;
    factory_lock();
    size = factory.worker_num;
    factory_unlock();
    return size;
}


void *worker_main(void *arg)
{
    worker_t *wkr = (worker_t *) arg;
    task_t *tsk = NULL;
    int iret = 0;

    pthread_detach(pthread_self());
    pthread_setspecific(factory.key, wkr);

    wkr->idx = pthread_self();
    M_cirvnl(worker_wait(wkr));

    wkr->state = WORKER_START;

    factory_lock();
    LIST_INSERT_HEAD(&factory, worker_head,  wkr, node);
    factory.worker_num++;
    factory_unlock();

    log_info("%lx start", wkr->idx);

    while(factory.state != FACTORY_OVER)
    {
        factory_task_pop(wkr, &tsk);
        iret = tsk->thread_main.func(tsk->data);
        log_debug("%lx process function:(%s) return:%d.", wkr->idx, tsk->thread_main.name, iret);
        worker_return(tsk);
        log_info("%lx return ok", wkr->idx);
        iret = factory_worker_count();
        log_info("%lx current worker num:%d", wkr->idx, iret);
        if (iret > factory.worker_min)
            break;
    }

    log_info("%lx stop", wkr->idx);

    return NULL;
}

int worker_create()
{
    worker_t *wkr = NULL;

    M_cvril((wkr = (worker_t *)M_alloc(sizeof(worker_t))), "malloc worker error, %m.");

    pthread_mutex_init(&(wkr->mtx), NULL);
    pthread_cond_init(&(wkr->cnd), NULL);

    wkr->state = WORKER_READY;

    if(pthread_create(&(wkr->idx), NULL, worker_main, (void *)wkr) != 0)
    {
        log_error("pthread_create error.%m");
        return MRT_ERR;
    }

    while(wkr->state == WORKER_READY)
    {
        worker_wakeup(wkr);
        usleep(100);
        continue;
    }

    log_debug("create success, worker:(%lu), worker num:%x", wkr->idx, factory.worker_num);

    return MRT_SUC;
}

void *worker_master(void *arg)
{
    worker_t *wkr = (worker_t *)arg;

    pthread_detach(pthread_self());
    pthread_setspecific(factory.key, wkr);

    wkr->idx = pthread_self();
    wkr->state = WORKER_START;

    while(factory.state == WORKER_START)
    {
        factory_lock();
        //如果任务数除2比工人数少，就不算忙, 就是任务数是否大于工人数量的2倍
        if((factory.task_num >> 1 ) < factory.worker_num)
            factory.busy = 0;
        else
        {
            factory.busy = 1;
            if(factory.worker_num < factory.worker_max)
            {
                log_info("task_num:%d, worker_num:%d, max:%d", factory.task_num, factory.worker_num, factory.worker_max);
                if(worker_create() == MRT_ERR)
                    log_error("create process center worker error.");
            }
        }
        factory_unlock();
        sleep(1);
    }

    return NULL;
}






void factory_deinit()
{
    factory_lock();

    factory.state = FACTORY_OVER;

    factory.worker_max = 0;

    while(factory.worker_num)
    {
        log_info("current worker num:%d\n", factory.worker_num);
        //向worker_center中所有worker发信号
        factory_wakeup_all();
        //解锁
        factory_unlock();
        //休息一秒
        sleep(1);
        //加锁，检测worker数量
        factory_lock();
    }
    //解锁
    factory_unlock();
}






void *worker_deinit(void *arg)
{
    worker_t *wkr = (worker_t *) arg;

    factory_lock();

    LIST_REMOVE(&factory, worker_head, wkr, node);

    M_free(wkr);
    master_wakeup();
    factory.worker_num--;

    factory_unlock();

    return NULL;
}


int factory_start()
{
    worker_t *wkr = &factory.master;
    int i=0;

    for(i=0; i< factory.worker_min; i++)
    {
        M_ciril(worker_create(), "create process center worker %d error.", i);
    }

    if(pthread_create(&(wkr->idx), NULL, worker_master, (void *)wkr) != 0)
    {
        log_error("pthread_create error.%m");
        return MRT_ERR;
    }

    factory.state = FACTORY_START;

    return MRT_SUC;
}


