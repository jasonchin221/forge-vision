#ifndef __FV_LIST_H__
#define __FV_LIST_H__

typedef struct _fv_list_head_t {
    struct _fv_list_head_t  *next;
    struct _fv_list_head_t  *prev;
} fv_list_head_t;

#define FV_LIST_HEAD_INIT(name) { &(name), &(name) }

#define FV_LIST_HEAD(name) \
	fv_list_head_t name = FV_LIST_HEAD_INIT(name)

static inline void 
FV_INIT_LIST_HEAD(fv_list_head_t *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void 
_fv_list_add(fv_list_head_t *new,
			      fv_list_head_t *prev,
			      fv_list_head_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void 
fv_list_add(fv_list_head_t *new, fv_list_head_t *head)
{
	_fv_list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void 
fv_list_add_tail(fv_list_head_t *new, fv_list_head_t *head)
{
	_fv_list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void 
_fv_list_del(fv_list_head_t * prev, fv_list_head_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void 
_fv_list_del_entry(fv_list_head_t *entry)
{
	_fv_list_del(entry->prev, entry->next);
}

static inline void 
fv_list_del(fv_list_head_t *entry)
{
	_fv_list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int 
fv_list_is_last(const fv_list_head_t *list,
				const fv_list_head_t *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int 
fv_list_empty(const fv_list_head_t *head)
{
	return head->next == head;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define fv_list_entry(ptr, type, member) \
	fv_container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define fv_list_first_entry(ptr, type, member) \
	fv_list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define fv_list_last_entry(ptr, type, member) \
	fv_list_entry((ptr)->prev, type, member)

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_struct within the struct.
 */
#define fv_list_next_entry(pos, member) \
	fv_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_struct within the struct.
 */
#define fv_list_prev_entry(pos, member) \
	fv_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define fv_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define fv_list_for_each_entry(pos, head, member)				\
	for (pos = fv_list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = fv_list_next_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe 
 *          against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define fv_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = fv_list_first_entry(head, typeof(*pos), member),	\
		n = fv_list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = fv_list_next_entry(n, member))



#endif
