#ifndef	__LIST_FUNC_H__
#define	__LIST_FUNC_H__

typedef struct list_node list_node_t;
struct list_node {
    list_node_t *prev;
    list_node_t *next;
    void *dat;
};

typedef struct list_head list_head_t;
struct list_head{
    list_node_t *first;
    list_node_t *last;
    uint32_t size;
};

#define	LIST_INIT(list, head) \
    (list)->head.first = NULL;\
    (list)->head.last = NULL;\
    (list)->head.size = 0;

#define LIST_NODE_INIT(elm, node) \
    (elm)->node.prev = NULL; \
    (elm)->node.next = NULL; \
    (elm)->node.dat = (elm);


#define	LIST_INSERT_HEAD(list, head, elm, node) 			\
    LIST_NODE_INIT(elm, node); \
    if (((elm)->node.next = (list)->head.first) != NULL)	\
        (list)->head.first->prev = &((elm)->node);		\
    else \
        (list)->head.last = &((elm)->node); \
    (list)->head.first = &((elm)->node); \
    (elm)->node.prev = (list)->head.first; \
    (list)->head.size++;

#define	LIST_INSERT_TAIL(list, head, elm, node) 			\
    LIST_NODE_INIT(elm, node); \
    if(((elm)->node.prev = (list)->head.last)) \
        ((list)->head.last)->next = &(elm)->node; \
    if ((list)->head.first == NULL) \
        (list)->head.first = &(elm)->node; \
    (list)->head.last = &(elm)->node; \
    (list)->head.size++;

#define LIST_REMOVE_HEAD(list, head) \
    if (((list)->head.first = (list)->head.first->next) == NULL) \
        (list)->head.last = (list)->head.first; \
    else \
        ((list)->head.first)->prev = (list)->head.first; \
    (list)->head.size--;

#define LIST_REMOVE_TAIL(list, head) \
    if ((list)->head.last == (list)->head.first) \
    { \
        LIST_INIT(list, head); \
    }else \
    {   \
        (list)->head.last = (list)->head.last->prev; \
        (list)->head.last->next = NULL; \
        (list)->head.size--; \
    }

#define	LIST_REMOVE(list, head, elm, node) \
    do {      \
        if (((elm)->node.prev == (elm)->node.next) && ((elm)->node.prev == NULL)) \
            break; \
        if ((list)->head.first == &((elm)->node))           \
        { \
            LIST_REMOVE_HEAD(list, head);           \
            break; \
        } \
        if((list)->head.last == &((elm)->node)) \
        {\
            LIST_REMOVE_TAIL(list, head); \
            break; \
        }\
        (elm)->node.next->prev = (elm)->node.prev;	\
        (elm)->node.prev->next = (elm)->node.next;     \
        (elm)->node.prev = (elm)->node.next = NULL; \
        (list)->head.size--; \
    } while(0)

#define	LIST_FOREACH(var, node, list, head)	\
        for ((var) = ((list)->head.first)? ((list)->head.first)->dat: NULL;		    \
             (var);							    \
             (var) = ((var)->node.next) ? ((var)->node.next)->dat: NULL)

#define	LIST_EMPTY(list,head)		((list)->head.first == NULL)
#define	LIST_FIRST(list, head)		((list)->head.first) ? ((list)->head.first)->dat: NULL
#define	LIST_LAST(list, head)		((list)->head.last) ? ((list)->head.last)->dat: NULL
#define	LIST_SIZE(list, head)		((list)->head.size)

#endif

