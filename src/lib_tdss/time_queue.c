#include "global.h"
#include "inet_event.h"
#include "time_queue.h"

static  void      __time_queue_up(time_queue_t*, int32_t hole_index, inet_task_t* e);
static  void      __time_queue_down(time_queue_t*, int32_t hole_index, inet_task_t* e);


int time_queue_init(time_queue_t *heap, int32_t size)
{
    p_zero(heap);

    M_cvril((heap->task = (inet_task_t **)M_alloc(size)), "mem_alloc error size:%d", size);

    heap->size = size;

    return MRT_OK;
}

void time_queue_deinit(time_queue_t *heap)
{
    if(heap)
    {
        M_free(heap->task);
        p_zero(heap);
    }
}

int time_queue_resize(time_queue_t *heap)
{
    int32_t size = 0;

    if(heap)
    {
        size = heap->size << 2 ;
        log_debug("size:%d", size);

        M_cvril((heap->task = (inet_task_t **)M_realloc(heap->task, size)), "mem_realloc error size:%u", size);

        heap->size = size;

        return MRT_OK;
    }

    return MRT_ERR;
}



int time_queue_push(time_queue_t *heap, inet_task_t *task)
{
    if(heap->num + 1 > heap->size)
    {
        M_ciril(time_queue_resize(heap), "minheap init error");
    }

    __time_queue_up(heap, heap->num++, task);

    return MRT_OK;
}

inet_task_t *time_queue_pop(time_queue_t *heap)
{
    inet_task_t *tsk = NULL;

    if(heap->num > 0)
    {
        tsk = *heap->task;
        __time_queue_down(heap, 0u, heap->task[--heap->num]);
        tsk->idx = -1;
    }

    return tsk;
}

int time_queue_delete(time_queue_t *heap, inet_task_t* task)
{
    if(-1 != task->idx)
    {
        inet_task_t *ntsk = heap->task[--heap->num];
        int32_t parent = (task->idx - 1) / 2;
        if (task->idx > 0 && (heap->task[parent]->timeout > ntsk->timeout))
            __time_queue_up(heap, task->idx, ntsk);
        else
            __time_queue_down(heap, task->idx, ntsk);
        task->idx = -1;
        return 0;
    }
    return -1;
}



//��������ʱ�������±����һ����Ȼ��ͨ������ĺ�����������λ��
void __time_queue_up(time_queue_t* heap, int32_t hole_index, inet_task_t* task)
{
    int32_t parent = (hole_index - 1) / 2;

    //������ڵ����Ҫ����Ľڵ㣬�ͽ���λ�ã�ֱ���ҵ�������Ҫ����ڵ�ĸ��ڵ㣬���ߵ����
    while(hole_index && (heap->task[parent]->timeout > task->timeout))
    {
        (heap->task[hole_index] = heap->task[parent])->idx = hole_index; //�������ƶ����Լ���λ��
        hole_index = parent; //��ǰ�����Ϊ���׵�����
        parent = (hole_index - 1) / 2; //���׵�����Ϊ�µĸ�������
    }

    //��û�и��ױ��Լ�С��ʱ��ǰ����Ϊ�Լ�������
    (heap->task[hole_index] = task)->idx = hole_index;
}

//task�������һ���ڵ�
void __time_queue_down(time_queue_t *heap, int32_t hole_index, inet_task_t* task)
{
    int32_t min_child = 2 * (hole_index + 1);

    //�ҵ�ǰ�ڵ�������ӽڵ�����С�ģ��ŵ���ǰλ�ã�ֱ��û���ӽڵ㣬����min_child
    //����heap->num��ʱ��, ��ʱֻ�ǵ�ǰ������·������
    while(min_child <= heap->num)
    {
        //����ֻѡ��С�Ľڵ㣬��Ϊû�˱���������������С�ģ����豣֤��
        min_child -= min_child == heap->num || (heap->task[min_child]->timeout >  heap->task[min_child - 1]->timeout);
        //��Ϊ�������ͨ��ǰ���ɾ���������ڵ㲻�����Ľڵ���, ֱ�ӶԵ�������,
        //��ͱ�֤�˲�����ά������ѵ���״��
        if(task->timeout <= heap->task[min_child]->timeout)
            break;
        (heap->task[hole_index] = heap->task[min_child])->idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }

    //��Ϊ�������·һֱ�����ƶ������һ���ˣ�������ȫ�ֵ����һ��Ԫ��ȥ���Ǹ���λ
    (heap->task[hole_index] = task)->idx = hole_index;
}


