#include "global.h"

process_center_t pc;

#define process_center_lock() pthread_mutex_lock(&pc.mtx)
#define process_center_wakeup() pthread_cond_signal(&pc.cnd)
#define process_center_wakeup_all() pthread_cond_broadcast(&pc.cnd)
#define process_center_wait() pthread_cond_wait(&pc.cnd, &pc.mtx)
#define process_center_unlock() pthread_mutex_unlock(&pc.mtx)


int process_center_init(int wkr_max, int wkr_min, callback_t proc)
{
    s_zero(pc);

    pthread_mutex_init(&pc.mtx, NULL);

    pthread_cond_init(&pc.cnd, NULL);

    pc.worker_max = wkr_max;
    pc.worker_min = wkr_min;
    pc.proc = proc;

    M_list_init(&pc);

    return MRT_SUC;
}


int process_center_push(task_t *tsk)
{
    M_cpvril(tsk);

    process_center_lock();

    M_list_insert_tail(&pc, tsk);

    pc.task_num++;

    process_center_wakeup();

    process_center_unlock();

    return MRT_SUC;
}

int process_center_pop(task_t **ntsk)
{
    process_center_lock();

    while(!pc.task_num || M_list_empty(&pc))
    {
        //如果工人个数大于最小工人数当前线程就退出
        if(pc.worker_num > pc.worker_min && !pc.busy)
        {
            log_info("process center will close, worker num:%d, min:%d, max:%d.",
                     pc.worker_num, pc.worker_min, pc.worker_max);
            pc.worker_num--;
            process_center_unlock();
            return MRT_ERR;
        }

        log_debug("Process Center Current is empty.");
        process_center_wait();
    }

    *ntsk = M_list_first(&pc);

    M_list_remove_head(&pc);

    process_center_unlock();

    return MRT_SUC;
}


int process_center_check()
{
    int iret = 0;
    process_center_lock();

    if((pc.task_num >> 2 ) < pc.worker_num)
    {
        pc.busy = 0;
    }
    else
    {
        pc.busy = 1;
        if(pc.worker_num < pc.worker_max)
        {
            iret = 1;
        }
    }

    process_center_unlock();

    return iret;
}



int process_center_loop()
{
    int iret = MRT_SUC;
    task_t *tsk = NULL;

    process_center_lock();

    if(pc.worker_num >= pc.worker_max)
    {
        log_error("Max Process Center worker num:%d, current is %d", pc.worker_max, pc.worker_num);
        return MRT_ERR;
    }
    pc.worker_num++;

    process_center_unlock();

    while(!process_center_pop(&tsk))
    {
        //调用task指定的处理函数去处理
        if(pc.proc.argv == NULL)
            iret = pc.proc.func();
        else
            iret = pc.proc.func(tsk);

        log_debug("process function:(%s) return:%d.", pc.proc.name, iret);
    }

    process_center_lock();

    if(pc.worker_num < pc.worker_min)
    {
        log_error("Min Process Center Worker num:%d, current is %d", pc.worker_min, pc.worker_num);
        return MRT_ERR;
    }
    pc.worker_num--;

    process_center_unlock();

    return MRT_SUC;
}



int process_center_deinit()
{
    process_center_lock();

    pc.worker_max = 0;

    while(pc.worker_num)
    {
        log_info("current worker num:%d\n", pc.worker_num);
        //向worker_center中所有worker发信号
        process_center_wakeup_all();
        //解锁
        process_center_unlock();
        //休息一秒
        sleep(1);
        //加锁，检测worker数量
        process_center_lock();
    }
    //解锁
    process_center_unlock();

    return MRT_SUC;
}


