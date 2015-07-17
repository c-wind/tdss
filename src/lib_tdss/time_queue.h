#ifndef __TIME_QUEUE_H__
#define __TIME_QUEUE_H__


//��ʼ����С��
int     time_queue_init(time_queue_t *heap, int32_t size);
//�ͷ���С���еı�������ռ���
void    time_queue_deinit(time_queue_t *heap);

//��������Ԫ��
int       time_queue_push(time_queue_t *heap, inet_task_t *);

//ȡ��������С��Ԫ��
inet_task_t    *time_queue_pop(time_queue_t *heap);

//ɾ����������Ԫ��
int       time_queue_delete(time_queue_t *heap, inet_task_t *);


//��ʼ��׼����ѵ�Ԫ��
#define time_queue_elem_init(task) (task->idx = -1)
//�жϵ�ǰ���Ƿ�Ϊ��
#define time_queue_empty(heap) (0 == (heap)->num)
//���ص�ǰ��Ԫ�ظ���
#define time_queue_num(heap) ((heap)->num)
//���ض�����СԪ��ָ�룬���Ӷ���ȡ��
#define time_queue_min(heap) (((heap)->num) ? *((heap)->task) : NULL)

#endif

