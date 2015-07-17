#ifndef __MINHEAP_H__
#define __MINHEAP_H__




//初始化最小堆
int     minheap_init(minheap_t *heap, int32_t size);
//释放最小堆中的变量，清空计数
void    minheap_deinit(minheap_t *heap);

//向堆中添加元素
int       minheap_push(minheap_t *heap, task_t *);

//取出堆中最小的元素
task_t    *minheap_pop(minheap_t *heap);

//删除堆中任意元素
int       minheap_delete(minheap_t *heap, task_t *);


//初始化准备入堆的元素
#define minheap_elem_init(task) (task->idx = -1)
//判断当前堆是否为空
#define minheap_empty(heap) (0 == (heap)->num)
//返回当前堆元素个数
#define minheap_num(heap) ((heap)->num)
//返回堆中最小元素指针，不从堆中取出
#define minheap_min(heap) (((heap)->num) ? *((heap)->task) : NULL)

#endif

