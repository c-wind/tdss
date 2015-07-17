#ifndef	__LIST_FUNC_H__
#define	__LIST_FUNC_H__


/*
 * Tail queue definitions.
 */
#define	M_list_head(type)					\
    type *_first;		/* first element */		\
type *_last;	/* addr of last next element */

#define	M_list_head_initalizer(head) \
{ NULL, (head)._first }

#define	M_list_entry(type)					\
    type *_next;		/* next element */		\
type *_prev;	/* address of previous next element */

/*
 * Tail queue functions.
 */
#define	M_list_init(head) 						\
    (head)->_first = NULL;					        \
(head)->_last = (head)->_first;

#define	M_list_insert_head(head, elm) 			\
    if (((elm)->_next = (head)->_first) != NULL)	\
        (head)->_first->_prev =	 (elm);		\
    else								            \
        (head)->_last = (elm);		        \
    (head)->_first = (elm);					        \
    (elm)->_prev = (head)->_first;

#define	M_list_insert_tail(head, elm) 			\
    (elm)->_next = NULL;					\
    if(((elm)->_prev = (head)->_last)) \
        ((head)->_last)->_next = (elm); \
    if ((head)->_first == NULL) \
        (head)->_first = (elm); \
    (head)->_last = (elm);

#define M_list_remove_head(head) \
    if (((head)->_first = (head)->_first->_next) == NULL) \
        (head)->_last = (head)->_first; \
    else \
        ((head)->_first)->_prev = (head)->_first;

#define M_list_remove_tail(head) \
    if ((head)->_last == (head)->_first) \
    { \
        M_list_init(head); \
    } \
    else \
    {   \
        (head)->_last = (head)->_last->_prev; \
        (head)->_last->_next = NULL; \
    }

#define	M_list_remove(head, elm) \
    do {      \
        if ((head)->_first == (elm))           \
        { \
            M_list_remove_head(head);           \
            break; \
        } \
        if((head)->_last == elm) \
        {\
            M_list_remove_tail(head); \
            break; \
        }\
        (elm)->_next->_prev = (elm)->_prev;	\
        (elm)->_prev->_next = (elm)->_next;     \
    } while(0)

#define	M_list_foreach(var, head)			\
    for ((var) = ((head)->_first);		    \
         (var);							    \
         (var) = ((var)->_next))

/*
 * Tail queue access methods.
 */
#define	M_list_empty(head)		((head)->_first == NULL)
#define	M_list_first(head)		((head)->_first)
#define	M_list_last(head)		((head)->_last)

#endif

