#include "global.h"

#if 0

#define M_mem_align(size) (((size) + (MAX_ALIGN) - 1) & ~((unsigned int) (MAX_ALIGN) -1))

inline static int block_alloc(worker_t *wkr, int64_t size, block_t **blk);
inline static block_t *memory_addr_check(worker_t *wkr, void *data);
int memory_status();


static void *mem_line_alloc(worker_t *wkr, int64_t size, int line, char *func)
{
    block_list_t *blst = NULL;
    block_t *nblk = NULL;

    if(size > M_32KB)
    {
        M_cvrvs((nblk = malloc(sizeof(*nblk) + size)), "malloc error, %m");
        memset(nblk, 0, sizeof(*nblk) +size);
        nblk->data = nblk + 1;
        nblk->stat = MEM_ONCE | MEM_TRUE;
        nblk->mem_size = 1;

#ifdef MEMORY_DEBUG
        blst = &wkr->memory_pool.used_blst[M_1KB];
#endif

    }
    else
    {
        blst = &wkr->memory_pool.free_blst[(size - 1) >> 5];
        if(blst->blk_sum == 0)
        {
            M_cirvs(block_alloc(wkr, blst->blst_width, &nblk), "alloc new memory error");
            nblk->blst_id = blst->blst_id;
            nblk->stat = MEM_TRUE;
        }
        else
        {
            nblk = M_list_first(blst);
            M_list_remove_head(blst);
            blst->blk_sum--;
            memset(nblk->data, 0, blst->blst_width);
        }
#ifdef MEMORY_DEBUG
        blst = &wkr->memory_pool.used_blst[(size - 1) >> 5];
#endif
    }

#ifdef MEMORY_DEBUG
    /* 此处的blst是used_blst的list */
    M_list_insert_head(blst, nblk);
    blst->blk_sum++;
    strcpy(nblk->func, func);
    nblk->line = line;
#endif

    return (nblk->data);
}

void *memory_realloc(worker_t *wkr, void *old_data, int64_t size, int line, char *func)
{
    void *data;

    block_t *nblk, *oblk;

    if(!old_data)
    {
        return mem_line_alloc(wkr, size, line, func);
    }

    oblk = old_data - sizeof(block_t);

    M_cvrvnl((data = mem_line_alloc(wkr, size, line, func)));

    nblk = data - sizeof(block_t);

    memcpy(nblk->data, oblk->data, oblk->mem_size > size ? size : oblk->mem_size);

    memory_free(wkr, old_data, line, func);

    return data;
}


void memory_free(worker_t *wkr, void *data, int line, char *func)
{
    block_t *blk;
    block_list_t *blst;

    if(!(blk = memory_addr_check(wkr, data)))
    {
        log_error("No found memory addr:%p", data);
#ifdef MEMORY_DEBUG
        memory_status();
#endif
        log_backtrace();
        exit(0);
    }


#ifdef MEMORY_DEBUG
    blst = &wkr->memory_pool.used_blst[blk->blst_id];
    //M_blst_lock(blst);
	if (((blk)->_next) != NULL)
		(blk)->_next->_prev = (blk)->_prev;
	else
		(blst)->_last = (blk)->_prev;

    if((blst)->_last == (blk))
        (blst)->_last = NULL;
    if(((blk)->_prev) == (blk))
        (blst)->_first = NULL;

	(blk)->_prev = (blk)->_next;

    //M_list_remove(blst, blk);
    //M_blst_unlock(blst);
#endif

    if(blk->stat & MEM_ONCE)
    {
        *(char *)data = 0;
        free(blk);
    }
    else
    {
        blst = &wkr->memory_pool.free_blst[blk->blst_id];
        //M_blst_lock(blst);
        M_list_insert_head(blst, blk);
        blst->blk_sum++;
        //M_blst_unlock(blst);
        *(char *)data = 0;
    }
}



inline int block_alloc(worker_t *wkr, int64_t size, block_t **blk)
{
    int64_t sum = size + sizeof(block_t);

    if(wkr->memory_pool.master->free < M_128KB)
    {
        if( wkr->memory_pool.slave == NULL)
        {
            memory_t *mry;

            M_cvril((mry = malloc(M_16MB + sizeof(memory_t))), "malloc big memory error, error:%m");

            mry->free = M_16MB;
            mry->used = 0;
            mry->data = mry + 1;

            M_list_insert_head(wkr->memory_pool.memory_list, mry);

            wkr->memory_pool.slave = mry;

        }

        if(wkr->memory_pool.master->free < M_32KB)
        {
            wkr->memory_pool.master = wkr->memory_pool.slave;
            wkr->memory_pool.slave = NULL;
        }

    }

    *blk = wkr->memory_pool.master->data + wkr->memory_pool.master->used;
    memset(*blk, 0, sizeof(**blk) + size);
    (*blk)->data = *blk + 1;
    (*blk)->mem_size = size;

    wkr->memory_pool.master->used += sum;
    wkr->memory_pool.master->free -= sum;

    return MRT_SUC;
}


inline static block_t *memory_addr_check(worker_t *wkr, void *data)
{
    block_t *blk;

    blk = data - sizeof(block_t);

    if(!(blk->stat & MEM_TRUE) || blk->blst_id > M_1KB)
        return NULL;

#ifdef MEMORY_DEBUG
    int i=0;
    block_t *tblk;

    for(; i< M_1KB + 1; i++)
        M_list_foreach(tblk, &wkr->memory_pool.used_blst[i])
        {
            if(tblk->data == data)
                return blk;
        }
    return NULL;
#endif

    return blk;
}



/*
 * 说明：
 *      block list 的第1024个是为超大内存保留的
 *
 *
 */
int memory_pool_init(worker_t *wkr)
{
    int i=0;
    memory_t *mry;

    s_zero(wkr->memory_pool);

    if(!(wkr->memory_pool.free_blst = calloc(M_1KB + 1, sizeof(block_list_t))))
    {
        printf("%s:%d calloc free_blst error, %m.", __func__, __LINE__);
        return MRT_ERR;
    }

    if(!(wkr->memory_pool.memory_list = calloc(1, sizeof(memory_list_t))))
    {
        printf("%s:%d calloc memory_list error, %m.", __func__, __LINE__);
        return MRT_ERR;
    }

    M_list_init(wkr->memory_pool.memory_list);

#ifdef MEMORY_DEBUG
    if(!(wkr->memory_pool.used_blst = calloc(M_1KB + 1, sizeof(block_list_t))))
    {
        printf("%s:%d calloc used_blst error, %m.", __func__, __LINE__);
        return MRT_ERR;
    }
#endif

    for(i=0; i< M_1KB + 1; i++)
    {
        wkr->memory_pool.free_blst[i].blst_id = i;
        wkr->memory_pool.free_blst[i].blst_width = (i + 1) << 5;
        M_list_init(&wkr->memory_pool.free_blst[i]);
        pthread_mutex_init(&(wkr->memory_pool.free_blst[i].mtx), NULL);

#ifdef MEMORY_DEBUG
        wkr->memory_pool.used_blst[i].blst_id = i;
        wkr->memory_pool.used_blst[i].blst_width = (i + 1) << 5;
        M_list_init(&wkr->memory_pool.used_blst[i]);
        pthread_mutex_init(&(wkr->memory_pool.used_blst[i].mtx), NULL);
#endif

    }

    M_cvrinl((wkr->memory_pool.master = mry = malloc(M_16MB + sizeof(memory_t))));
    wkr->memory_pool.master->free = M_16MB;
    wkr->memory_pool.master->used = 0;
    wkr->memory_pool.master->data = wkr->memory_pool.master + 1;

    M_list_insert_head(wkr->memory_pool.memory_list, mry);

    return MRT_OK;
}


int memory_status(worker_t *wkr)
{
#ifdef MEMORY_DEBUG
    int i=0;
    block_t *tblk;
    block_list_t *blst;

    log_info(" ***************** Memory Info Begin ***************\n");
    log_info(" ----------------- Memory Used Begin ---------------");

    for(i=0; i< M_1KB + 1; i++)
    {
        blst = &wkr->memory_pool.used_blst[i];
        //M_blst_lock(blst);
        M_list_foreach(tblk, blst)
        {
            log_info("Function:%s, Line:%d, alloc size:%u",
                     tblk->func, tblk->line, tblk->mem_size);
        }
        //M_blst_unlock(blst);
    }

    log_info(" ----------------- Memory Used end ---------------");
    log_info(" ----------------- Memory Free Begin ---------------");
    for(i=0; i< M_1KB + 1; i++)
    {
        blst = &wkr->memory_pool.free_blst[i];
        //M_blst_lock(blst);

        M_list_foreach(tblk, blst)
        {
            log_info("Function:%s, Line:%d, alloc size:%u",
                     tblk->func, tblk->line, tblk->mem_size);
        }
        //M_blst_unlock(blst);
    }
    log_info(" ----------------- Memory Free End ---------------");
    log_info(" ***************** Memory Info End ***************\n");

#endif
    return MRT_SUC;
}



int memory_pool_deinit(worker_t *wkr)
{
    memory_t *mry, *old = NULL;

    memory_status(wkr);

#ifdef MEMORY_DEBUG
    free(wkr->memory_pool.used_blst);
#endif
    free(wkr->memory_pool.free_blst);

    M_list_foreach(mry, wkr->memory_pool.memory_list)
    {
        if(old)
            free(old);
        old = mry;
    }

    if(old)
        free(old);

    free(wkr->memory_pool.memory_list);

    return MRT_OK;
}


int memory_pool_reset(worker_t *wkr)
{
    M_cirinl(memory_pool_deinit(wkr));
    M_cirinl(memory_pool_init(wkr));
    return MRT_SUC;
}








#ifdef MEM_POOL_TEST


int sys_atest()
{
    int ret;
    char *tstr[9999];
    int i=0, j=0, k=0, tms =0;
    struct timeval t1, t2;
    struct timezone tz;
    uint64_t vi;

    memset(&t1, 0 , sizeof(struct timeval));
    memset(&t2, 0 , sizeof(struct timeval));

    gettimeofday(&t1, &tz);

    printf("\n\t\tSystem Malloc & Free Test!\n"
           "\t----------------------------------------------------\n");

    for(; j< 1000; j++)
    {
        k=0;
        //printf("NO.%d Begin run system malloc...\n", j);
        for(i=1; i< 9999; i++)
        {
            if((i%37) == 0)
            {
                tstr[k] = malloc(i);
                k++;
                tms++;
            }

        }
        //printf("NO.%d Begin run system free...\n", j);
        for(k--; k >= 0; k--)
        {
            free(tstr[k]);
        }

    }

    ret = tms;

    gettimeofday(&t2,&tz);

    vi = (t2.tv_sec - t1.tv_sec) * 1000000;
    vi += t2.tv_usec;

    printf("System malloc run %d times. \tused time:%ld usec.\n", ret, vi - t1.tv_usec);

    return ret;
}



int mp_atest()
{
    int ret;
    char *tstr[9999];
    int i=0, j=0, k=0, tms=0;
    struct timeval t1, t2;
    struct timezone tz;
    uint64_t vi;

    memset(&t1, 0 , sizeof(struct timeval));
    memset(&t2, 0 , sizeof(struct timeval));

    gettimeofday(&t1, &tz);

    printf("\n\t\tMemory Pool Malloc & Free Test!\n"
           "\t----------------------------------------------------\n");

    for(; j< 1000; j++)
    {
        k=0;
        //printf("NO.%d Begin run memory pool malloc...\n", j);
        for(i=1; i< 9999; i++)
        {
            if((i%37) == 0)
            {
                tstr[k] = mem_line_alloc(i, __LINE__, (char *)__func__);
                k++;
                tms++;
            }

        }

        //printf("NO.%d Begin run memory pool free...\n", j);
        for(k--; k >= 0; k--)
        {
            memory_free(tstr[k], __LINE__, (char *)__func__);
        }

    }
    ret = tms;

    gettimeofday(&t2,&tz);

    vi = (t2.tv_sec - t1.tv_sec) * 1000000;
    vi += t2.tv_usec;

    printf("Memory pool malloc run %d times. \tused time:%ld usec.\n\n", ret, vi - t1.tv_usec);

    return ret;
}

int64_t disturb_id=0;

void *disturb()
{
    int i=0;
    char *vs[100];

    while(1)
    {
        for(i=0; i< 100; i++)
        {
            vs[i] = malloc(i);
            disturb_id++;
        }

        for(i=0; i<100; i++)
        {
            free(vs[i]);
        }

    }

    return NULL;
}







int main(int argc, char *argv[])
{

    if(memory_pool_init() == MRT_ERR)
    {
        printf("%s:%d mem pool init error.\n", __func__, __LINE__);
        return -1;
    }

    log_init("./", "mem_test", "debug");
    //mp_status();

    //pthread_create(&pid, NULL, disturb, NULL);

    /*
       sys_atest();

       mp_atest();

       printf("disturb %u times\n", disturb_id);
       */

    /*开debug测试 */

    char *pstr = M_alloc(1024);
    strcpy(pstr, "asdfasdfasdfasdfdsadfdsdaffdfsfsdafsadfsdff");
    char *ppstr = pstr + 10;

    char *ddv10 = M_alloc(100);
    char *ddv11 = M_alloc(11);


    M_free(ddv10);

    M_free(pstr);

    M_free(ddv11);

    memory_pool_destroy();


    return 0;

}

#endif


#endif

