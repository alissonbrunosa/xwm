#ifndef QUEUE_H
#define QUEUE_H

#define LINKED_LIST_NODE(type)  \
    type* next;                 \
    type* previous;             \

#define LINKED_LIST(type)       \
    struct {                    \
        type* first;            \
        type* last;             \
    }



#define LIST_INIT(list)         \
    do {                        \
        (list)->head = NULL;    \
        (list)->tail = NULL;    \
    } while (0)

#define LIST_FIRST(list)(list.first)
#define LIST_LAST(list) (list.last)
#define LIST_NEXT(element) ((element)->next)
#define LIST_PREVIOUS(element) ((element)->previous)

#endif // QUEUE_H
