#ifndef __MINHEAP_H__
#define __MINHEAP_H__




//��ʼ����С��
int     minheap_init(minheap_t *heap, int32_t size);
//�ͷ���С���еı�������ռ���
void    minheap_deinit(minheap_t *heap);

//��������Ԫ��
int       minheap_push(minheap_t *heap, task_t *);

//ȡ��������С��Ԫ��
task_t    *minheap_pop(minheap_t *heap);

//ɾ����������Ԫ��
int       minheap_delete(minheap_t *heap, task_t *);


//��ʼ��׼����ѵ�Ԫ��
#define minheap_elem_init(task) (task->idx = -1)
//�жϵ�ǰ���Ƿ�Ϊ��
#define minheap_empty(heap) (0 == (heap)->num)
//���ص�ǰ��Ԫ�ظ���
#define minheap_num(heap) ((heap)->num)
//���ض�����СԪ��ָ�룬���Ӷ���ȡ��
#define minheap_min(heap) (((heap)->num) ? *((heap)->task) : NULL)

#endif

