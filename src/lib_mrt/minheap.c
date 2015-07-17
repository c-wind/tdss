#include "global.h"

static  void      __minheap_up(minheap_t*, int32_t hole_index, task_t* e);
static  void      __minheap_down(minheap_t*, int32_t hole_index, task_t* e);


int minheap_init(minheap_t *heap, int32_t size)
{
    p_zero(heap);

    M_cvril((heap->task = (task_t **)M_alloc(size)), "mem_alloc error size:%d", size);

    heap->size = size;

    return MRT_SUC;
}

void minheap_deinit(minheap_t *heap)
{
    if(heap)
    {
        M_free(heap->task);
        p_zero(heap);
    }
}

int minheap_resize(minheap_t *heap)
{
    int32_t size = 0;

    if(heap)
    {
        size = heap->size << 2 ;
        log_info("size:%d", size);

        M_cvril((heap->task = (task_t **)M_realloc(heap->task, size)), "mem_realloc error size:%u", size);

        heap->size = size;

        return MRT_OK;
    }

    return MRT_ERR;
}



int minheap_push(minheap_t *heap, task_t *task)
{
    if(heap->num + 1 > heap->size)
    {
        M_ciril(minheap_resize(heap), "minheap init error");
    }

    __minheap_up(heap, heap->num++, task);

    return MRT_OK;
}

task_t *minheap_pop(minheap_t *heap)
{
    task_t *tsk = NULL;

    if(heap->num > 0)
    {
        tsk = *heap->task;
        __minheap_down(heap, 0u, heap->task[--heap->num]);
        tsk->idx = -1;
    }

    return tsk;
}

int minheap_delete(minheap_t *heap, task_t* task)
{
    if(-1 != task->idx)
    {
        task_t *ntsk = heap->task[--heap->num];
        int32_t parent = (task->idx - 1) / 2;
        if (task->idx > 0 && (heap->task[parent]->last > ntsk->last))
            __minheap_up(heap, task->idx, ntsk);
        else
            __minheap_down(heap, task->idx, ntsk);
        task->idx = -1;
        return 0;
    }
    return -1;
}



//插入数据时放在最下边最后一个，然后通过下面的函数找向上找位置
void __minheap_up(minheap_t* heap, int32_t hole_index, task_t* task)
{
    int32_t parent = (hole_index - 1) / 2;

    //如果父节点大于要插入的节点，就交换位置，直到找到不大于要插入节点的父节点，或者到最顶层
    while(hole_index && (heap->task[parent]->last > task->last))
    {
        (heap->task[hole_index] = heap->task[parent])->idx = hole_index; //将父亲移动到自己的位置
        hole_index = parent; //当前坐标改为父亲的坐标
        parent = (hole_index - 1) / 2; //父亲的坐标为新的父亲坐标
    }

    //当没有父亲比自己小的时候当前坐标为自己的坐标
    (heap->task[hole_index] = task)->idx = hole_index;
}

//task就是最后一个节点
void __minheap_down(minheap_t *heap, int32_t hole_index, task_t* task)
{
    int32_t min_child = 2 * (hole_index + 1);

    //找当前节点的两个子节点中最小的，放到当前位置，直到没有子节点，就是min_child
    //大于heap->num的时候, 这时只是当前这条线路到底了
    while(min_child <= heap->num)
    {
        //这里只选最小的节点，因为没人保存这个堆是左大右小的（不需保证）
        min_child -= min_child == heap->num || (heap->task[min_child]->last >  heap->task[min_child - 1]->last);
        //因为可能最后通过前面的删除操作最后节点不是最大的节点了, 直接对调就行了,
        //这就保证了不用总维护这个堆的形状了
        if(task->last <= heap->task[min_child]->last)
            break;
        (heap->task[hole_index] = heap->task[min_child])->idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }

    //因为上面的线路一直向上移动到最后一个了，这里用全局的最后一个元素去顶那个空位
    (heap->task[hole_index] = task)->idx = hole_index;
}

#if 0

int main()
{
    int i,j;
    task_t *tsk = NULL, *ftsk = NULL, *etsk = NULL;

    memset(&myheap, 0, sizeof(myheap));

    tsk = M_alloc(sizeof(*tsk) * 20);

    for(i=0; i< 20; i++)
    {
        //j = (i * 33) % 179;
        j=i;

        tsk[i].stat = j;
        tsk[i].last = j;
        printf("insert stat:%d last:%d\n", tsk[i].stat, tsk[i].last);

        minheap_push(&tsk[i]);
    }

    puts("");
    minheap_erase(&tsk[3]);
    for(i=0; i< heap->num; i++)
    {
        printf("idx:%d stat:%d last:%d\n", heap->task[i]->idx, heap->task[i]->stat, heap->task[i]->last);
    }

    /*
    puts("");
    minheap_erase(&tsk[5]);
    for(i=0; i< heap->num; i++)
    {
        printf("idx:%d stat:%d last:%d\n", heap->task[i]->idx, heap->task[i]->stat, heap->task[i]->last);
    }

    puts("");
    minheap_erase(&tsk[2]);
    for(i=0; i< heap->num; i++)
    {
        printf("idx:%d stat:%d last:%d\n", heap->task[i]->idx, heap->task[i]->stat, heap->task[i]->last);
    }

    */

    puts("");

    while(!minheap_empty(heap))
    {
        printf("idx:%d stat:%d last:%d\n",  heap->task[0]->idx, heap->task[0]->stat, heap->task[0]->last);
        minheap_erase(minheap_min(heap));
    }

    return 0;
}

#endif

