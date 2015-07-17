#ifndef __TIME_QUEUE_H__
#define __TIME_QUEUE_H__


//初始化最小堆
int     time_queue_init(time_queue_t *heap, int32_t size);
//释放最小堆中的变量，清空计数
void    time_queue_deinit(time_queue_t *heap);

//向堆中添加元素
int       time_queue_push(time_queue_t *heap, inet_task_t *);

//取出堆中最小的元素
inet_task_t    *time_queue_pop(time_queue_t *heap);

//删除堆中任意元素
int       time_queue_delete(time_queue_t *heap, inet_task_t *);


//初始化准备入堆的元素
#define time_queue_elem_init(task) (task->idx = -1)
//判断当前堆是否为空
#define time_queue_empty(heap) (0 == (heap)->num)
//返回当前堆元素个数
#define time_queue_num(heap) ((heap)->num)
//返回堆中最小元素指针，不从堆中取出
#define time_queue_min(heap) (((heap)->num) ? *((heap)->task) : NULL)

#endif

