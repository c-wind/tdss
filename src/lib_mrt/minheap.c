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



//��������ʱ�������±����һ����Ȼ��ͨ������ĺ�����������λ��
void __minheap_up(minheap_t* heap, int32_t hole_index, task_t* task)
{
    int32_t parent = (hole_index - 1) / 2;

    //������ڵ����Ҫ����Ľڵ㣬�ͽ���λ�ã�ֱ���ҵ�������Ҫ����ڵ�ĸ��ڵ㣬���ߵ����
    while(hole_index && (heap->task[parent]->last > task->last))
    {
        (heap->task[hole_index] = heap->task[parent])->idx = hole_index; //�������ƶ����Լ���λ��
        hole_index = parent; //��ǰ�����Ϊ���׵�����
        parent = (hole_index - 1) / 2; //���׵�����Ϊ�µĸ�������
    }

    //��û�и��ױ��Լ�С��ʱ��ǰ����Ϊ�Լ�������
    (heap->task[hole_index] = task)->idx = hole_index;
}

//task�������һ���ڵ�
void __minheap_down(minheap_t *heap, int32_t hole_index, task_t* task)
{
    int32_t min_child = 2 * (hole_index + 1);

    //�ҵ�ǰ�ڵ�������ӽڵ�����С�ģ��ŵ���ǰλ�ã�ֱ��û���ӽڵ㣬����min_child
    //����heap->num��ʱ��, ��ʱֻ�ǵ�ǰ������·������
    while(min_child <= heap->num)
    {
        //����ֻѡ��С�Ľڵ㣬��Ϊû�˱���������������С�ģ����豣֤��
        min_child -= min_child == heap->num || (heap->task[min_child]->last >  heap->task[min_child - 1]->last);
        //��Ϊ�������ͨ��ǰ���ɾ���������ڵ㲻�����Ľڵ���, ֱ�ӶԵ�������,
        //��ͱ�֤�˲�����ά������ѵ���״��
        if(task->last <= heap->task[min_child]->last)
            break;
        (heap->task[hole_index] = heap->task[min_child])->idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }

    //��Ϊ�������·һֱ�����ƶ������һ���ˣ�������ȫ�ֵ����һ��Ԫ��ȥ���Ǹ���λ
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

