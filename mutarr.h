#ifndef MUTARR_H
#define MUTARR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* (de)construct the element pointed to by `buf` */
typedef void (*ma_callb)(void *buf, void *userp);
/* compare the elements pointed to by `a` and `b` by subtracting them */
typedef int (*ma_cmpcb)(const void *a, const void *b);
/* copy the element pointed to by `ref` into the buffer pointed to by `buf` */
typedef int (*ma_copycb)(void *buf, const void *ref);
/* return a malloc()ed string of `buf` in `ret` and its size, or -1 */
typedef int (*ma_tostrcb)(char **ret, const void *buf);

typedef void *(*ma_xalloc)(void *ptr, size_t nmemb, size_t size);
typedef void (*ma_xdealloc)(void *ptr);

#define mutarr(type)                                       \
	{                                                  \
		struct mutarrb s;                          \
		size_t len;       /* number of elements */ \
		type *buf;        /* array buffer */       \
		type *ptr;                                 \
		type val;                                  \
	}

struct mutarrb {
	size_t alloc;
	size_t len;
	size_t sz;
	void *buf;
	ma_callb ctor;
	ma_callb dtor;
	void *userp;
	ma_cmpcb cmp;
	ma_tostrcb tostr;
};

/* `struct mutarr x` is a void *-array */
struct mutarr mutarr(void *);

/* forward declarations */
int ma_swap(struct mutarrb *a, struct mutarrb *b);
void *ma_fullswap(struct mutarrb *a, struct mutarrb *b);
void *ma_splice(struct mutarrb *ma, int offset, size_t len, const void *data,
		size_t dlen);
int ma_insertsorted(struct mutarrb *ma, const void *data);
int ma_copy(struct mutarrb *ma, int offset, const void *data, size_t dlen,
	    ma_copycb copy);
int ma_index(struct mutarrb *ma, int offset, const void *data);
int ma_rindex(struct mutarrb *ma, int offset, const void *data);
int ma_search(struct mutarrb *ma, int start, const void *data);
int ma_xchg(struct mutarrb *ma, int a, int b);
int ma_reverse(struct mutarrb *ma);
int ma_join(struct mutarrb *ma, char **ret, const char *sep);
static inline int ma_shrinktofit(struct mutarrb *ma);

extern ma_xalloc ma_xrealloc;
extern ma_xdealloc ma_xfree;

/**
 * ma_setrealloc() - set a custom allocator function
 * @routine: replacement for realloc(), with the latter parameters like calloc()
 */
void ma_setrealloc(ma_xalloc routine);

/**
 * tr_setfree() - set a custom deallocator function
 * @routine: replacement for free()
 */
void ma_setfree(ma_xdealloc routine);

/**
 * ma_init() - initialize an array
 * @ma: typed pointer to the mutable array
 * @hint: count of elements to allocate memory for
 *
 * Return: When successful 1, or otherwise 0 if ma_grow() failed. Always returns
 *	1 if @hint is 0.
 *
 * Note!: Do not access elements that are not added to the array yet, only those
 *	whose position is below @ma->len. Also, when specifying indices for any
 *	mutarr function, negative indices select elements from the end of the
 *	array and if an index is beyond any bound, then it is truncated.
 */
#define ma_init(ma, hint)                                \
	((ma)->s.alloc = (ma)->s.len = (ma)->len = 0,    \
	 (ma)->s.sz = sizeof(*(ma)->buf),                \
	 (ma)->s.ctor = (ma)->s.dtor = NULL,             \
	 (ma)->s.cmp = NULL,                             \
	 (ma)->s.tostr = NULL,                           \
	 (ma)->s.buf = (ma)->s.userp = (ma)->buf = NULL, \
	 hint ? ma_grow((ma), (hint)) : 1)

/**
 * ma_release() - release an array
 * @ma: typed pointer to the mutable array
 *
 * All elements are deleted, the buffer is released and @ma is reinitialized
 * with ma_init().
 */
#define ma_release(ma)                \
	do {                          \
		ma_freebuf(&(ma)->s); \
		ma_init((ma), 0);     \
	} while (0)

/**
 * ma_attach() - attach a buffer
 * @ma: typed pointer to the mutable array
 * @nbuf: pointer to a malloc()ed buffer
 * @nlen: number of elements pointed to by @nbuf
 * @nalloc: number of elements the buffer can hold
 *
 * The buffer @nbuf is henceforth owned by @ma and cannot be relied upon anymore
 * and also must not be free()d directly.
 */
#define ma_attach(ma, nbuf, nlen, nalloc)       \
	do {                                    \
		ma_freebuf(&(ma)->s);           \
		(ma)->s.buf = (ma)->buf = nbuf; \
		(ma)->s.len = (ma)->len = nlen; \
		(ma)->s.alloc = nalloc;         \
	} while (0)

/**
 * ma_detach() - detach an array's buffer
 * @ma: typed pointer to the mutable array
 * @ret: pointer that receives @ma's length, can be NULL
 *
 * Return: The array buffer of @ma. If @ma's has no allocated memory, then NULL
 *	is returned. You have to free() the buffer, when you no longer need it.
 */
#define ma_detach(ma, ret)                                           \
	((ma)->ptr = (ma_detach)(&(ma)->s, (ret)), (ma)->buf = NULL, \
	 (ma)->len = 0, (ma)->ptr)

/**
 * ma_setctor() - set a mutarr's constructor
 * @ma: typed pointer to the mutable array
 * @callb: routine that creates new elements
 */
#define ma_setctor(ma, callb) ((ma)->s.ctor = (callb), 0)

/**
 * ma_setdtor() - set a mutarr's deconstructor
 * @ma: typed pointer to the mutable array
 * @callb: routine that releases deleted elements
 */
#define ma_setdtor(ma, callb) ((ma)->s.dtor = (callb), 0)

 /**
 * ma_setuserp() - set a mutarr's user-defined value
 * @ma: typed pointer to the mutable array
 * @ptr: value to set
 */
#define ma_setuserp(ma, ptr) ((ma)->s.userp = (ptr), 0)

/**
 * ma_setdefval() - set a mutarr's default value
 * @ma: typed pointer to the mutable array
 * @...: value that is used for new elements if @ma->ctor is NULL
 */
#define ma_setdefval(ma, ...) ((ma)->val = (__VA_ARGS__), 0)

/**
 * ma_setcmp() - set a mutarr's compare routine
 * @ma: typed pointer to the mutable array
 * @callb: like the fourth parameter of qsort()
 */
#define ma_setcmp(ma, callb) ((ma)->s.cmp = (callb), 0)

/**
 * ma_settostr() - set a mutarr's tostring routine
 * @ma: typed pointer to the mutable array
 * @callb: routine that releases deleted elements
 */
#define ma_settostr(ma, callb) ((ma)->s.tostr = (callb), 0)

/**
 * ma_setcbs() - set a mutarr's callbacks
 * @ma: typed pointer to the mutable array
 * @ctorcb: constructor
 * @dtorcb: destructor
 * @cmpcb: compare routine
 * @tostrcb: stringify routine
 */
#define ma_setcbs(ma, ctorcb, dtorcb, cmpcb, tostrcb) \
	do {                                          \
		(ma)->s.ctor = (ctorcb);              \
		(ma)->s.dtor = (dtorcb);              \
		(ma)->s.cmp = (cmpcb);                \
		(ma)->s.tostr = (tostrcb);            \
	} while (0)

/**
 * ma_swap() - swap two arrays' buffers
 * @a: typed pointer to a mutable array
 * @b: typed pointer to another mutable array
 *
 * Return: When successful 1, or otherwise 0 when their type sizes mismatch. On
 *	failure nothing is swapped.
 */
#define ma_swap(a, b)                                   \
	((ma_swap)(&(a)->s, &(b)->s) ?                  \
	 ((a)->buf = (a)->s.buf, (a)->len = (a)->s.len, \
	  (b)->buf = (b)->s.buf, (b)->len = (b)->s.len, 1) : 0)

/**
 * ma_fullswap() - swap two arrays including their callbacks and default values
 * @a: typed pointer to a mutable array
 * @b: typed pointer to another mutable array
 *
 * Return: When successful 1, or otherwise 0 when their type sizes mismatch or
 *	realloc() failed. On failure nothing is swapped.
 */
#define ma_fullswap(a, b)                                                  \
	(((a)->ptr = (ma_fullswap)(&(a)->s, &(b)->s)) ?                    \
	 (*(a)->ptr = (a)->val, (a)->val = (b)->val, (b)->val = *(a)->ptr, \
	  (a)->buf = (a)->s.buf, (a)->len = (a)->s.len,                    \
	  (b)->buf = (b)->s.buf, (b)->len = (b)->s.len,                    \
	  ma_xfree((a)->ptr), 1) : 0)

/**
 * ma_reset() - delete all elements
 * @ma: typed pointer to the mutable array
 */
#define ma_reset(ma) ma_setlen((ma), 0)

/**
 * ma_grow() - allocate new memory
 * @ma: typed pointer to the mutable array
 * @extra: count of extra elements
 *
 * Ensure that @ma can hold at least @extra more elements.
 *
 * Return: When successful 1, or otherwise 0 if realloc() failed.
 */
#define ma_grow(ma, extra) \
	((ma_grow)(&(ma)->s, extra) ? ((ma)->buf = (ma)->s.buf, 1) : 0)

/**
 * ma_shrinktofit() - remove unused allocated memory
 * @ma: typed pointer to the mutable array
 *
 * Return: When successful 1, or otherwise 0 if realloc() failed.
 */
#define ma_shrinktofit(ma) \
	((ma_shrinktofit)(&(ma)->s) ? ((ma)->buf = (ma)->s.buf, 1) : 0)

/**
 * ma_avail() - get the amount of unused memory
 * @ma: typed pointer to the mutable array
 *
 * Return: The number of elements that can be added without reallocation.
 */
#define ma_avail(ma) ((ma)->s.alloc - (ma)->s.len)

/**
 * ma_insertp() - insert a new element
 * @ma: typed pointer to the mutable array
 * @offset: position where to insert
 *
 * Return: When successful a pointer to the new element, or otherwise NULL if
 *	ma_grow() failed.
 */
#define ma_insertp(ma, offset) ma_insertmanyp((ma), (offset), 1)

/**
 * ma_insertmanyp() - insert new elements
 * @ma: typed pointer to the mutable array
 * @offset: position where to insert
 * @nlen: number of new elements
 *
 * Return: When successful a pointer to the first new element, or otherwise NULL
 *	if @nlen is 0 or ma_grow() failed.
 */
#define ma_insertmanyp(ma, offset, nlen)                              \
	(((ma)->ptr = (ma_insertmanyp)(&(ma)->s, (offset), (nlen))) ? \
	 ((ma)->buf = (ma)->s.buf, (ma)->len = (ma)->s.len, (ma)->ptr) : NULL)

/**
 * ma_pushp() - append a new element
 * @ma: typed pointer to the mutable array
 *
 * Return: When successful a reference to the new element, or otherwise NULL if
 *	ma_grow() failed.
 */
#define ma_pushp(ma)                                                        \
	(((ma)->s.len == (ma)->s.alloc) ?                                   \
	 ma_grow((ma), 1) ? &(ma)->buf[(ma)->s.len++, (ma)->len++] : NULL : \
	 &(ma)->buf[(ma)->s.len++, (ma)->len++])

/**
 * ma_setlen() - adjust the amount of elements
 * @ma: typed pointer to the mutable array
 * @nlen: new number of elements @ma holds
 *
 * If @nlen is above @ma's current length, then new elements are added, either
 * by calling @ma->ctor() on them or by using @ma's default value. Respectively,
 * if that's not the case, then @ma->dtor() is called on all elements above the
 * new length. However, in either case the array is not reallocated and @nlen is
 * truncated to not exceed the allocation max.
 */
#define ma_setlen(ma, nlen)                                         \
	do {                                                        \
		size_t len = (nlen), i;                             \
		if ((ma)->s.len < len) {                            \
			if ((ma)->s.alloc < len)                    \
				len = (ma)->s.alloc;                \
			if ((ma)->s.ctor) {                         \
				for (i = (ma)->s.len; i < len; i++) \
					(ma)->s.ctor(&(ma)->buf[i], \
						   (ma)->s.userp);  \
			} else {                                    \
				for (i = (ma)->s.len; i < len; i++) \
					(ma)->buf[i] = (ma)->val;   \
			}                                           \
		} else if ((ma)->s.len > len) {                     \
			if ((ma)->s.dtor) {                         \
				for (i = len; i < (ma)->s.len; i++) \
					(ma)->s.dtor(&(ma)->buf[i], \
						   (ma)->s.userp);  \
			}                                           \
		}                                                   \
		(ma)->s.len = (ma)->len = len;                      \
	} while (0)

/**
 * ma_first() - get an array's first element
 * @ma: typed pointer to the mutable array
 *
 * Return: Reference to @ma's first element.
 */
#define ma_first(ma) ((ma)->buf[0])

/**
 * ma_last() - get an array's first element
 * @ma: typed pointer to the mutable array
 *
 * Return: Reference to @ma's last element.
 */
#define ma_last(ma) ((ma)->buf[(ma)->s.len - 1])

/**
 * ma_splice() - replace old elements with new ones
 * @ma: typed pointer to the mutable array
 * @offset: position where to start replacing
 * @rlen: number of old elements to delete
 * @data: pointer to new elements
 * @dlen: number of new elements to insert
 *
 * Return: When successful a pointer to the element following the last deleted
 *	element and to the first new element respectively, or otherwise NULL if
 *	@dlen is above 0 and ma_grow() failed.
 */
#define ma_splice(ma, offset, rlen, data, dlen)                       \
	(((ma)->ptr = (ma_splice)(&(ma)->s, (offset), (rlen), (data), \
				  (dlen))) ?                          \
	 ((ma)->buf = (ma)->s.buf, (ma)->len = (ma)->s.len,(ma)->ptr) : NULL)

/**
 * ma_insert() - insert an element
 * @ma: typed pointer to the mutable array
 * @offset: position where to insert
 * @...: value to insert
 *
 * Return: When successful 1, or otherwise 0 if ma_grow() failed.
 */
#define ma_insert(ma, offset, ...) \
	((ma_insertp((ma), (offset))) ? (*(ma)->ptr = (__VA_ARGS__), 1) : 0)

/**
 * ma_insertsorted() - insert an element into a sorted array
 * @ma: typed pointer to the sorted mutable array
 * @data: pointer to the data to insert
 *
 * Return: When successful 1, or otherwise 0 if @ma->cmp is NULL or ma_grow()
 *	failed.
 */
#define ma_insertsorted(ma, data)              \
	((ma_insertsorted)(&(ma)->s, (data)) ? \
	 ((ma)->buf = (ma)->s.buf, (ma)->len++, 1) : 0)

/**
 * ma_insertmany() - insert elements
 * @ma: typed pointer to the mutable array
 * @offset: position where to insert
 * @data: pointer to the elements
 * @dlen: number of elements to insert
 *
 * Return: When successful pointer to the first element inserted, or otherwise
 *	NULL if ma_grow() failed.
 */
#define ma_insertmany(ma, offset, data, dlen) \
	ma_splice((ma), (offset), 0, (data), (dlen))

/**
 * ma_delete() - delete an element
 * @ma: typed pointer to the mutable array
 * @offset: position where to delete
 *
 * Return: Pointer to the element following the deleted element.
 */
#define ma_delete(ma, offset) ma_splice((ma), (offset), 1, NULL, 0)

/**
 * ma_deletemany() - delete elements
 * @ma: typed pointer to the mutable array
 * @offset: position where to delete
 * @len: number of elements to delete
 *
 * Return: Pointer to the element following the last deleted element.
 */
#define ma_deletemany(ma, offset, len) ma_splice((ma), (offset), (len), NULL, 0)

/**
 * ma_push() - append an element
 * @ma: typed pointer to the mutable array
 * @...: value to push
 *
 * Return: When successful 1, or otherwise 0 if ma_grow() failed.
 *
 * Note!: @... is only a single value, it's denoted as varargs in order to cope
 *	with struct-literals, additionally, it is not evaluated if ma_push()
 *	failed (so no side effects).
 */
#define ma_push(ma, ...)                                                  \
	(((ma)->s.len == (ma)->s.alloc) ?                                 \
	 ma_grow((ma), 1) ?                                               \
	 ((ma)->buf[(ma)->s.len++] = (__VA_ARGS__), (ma)->len++, 1) : 0 : \
	 ((ma)->buf[(ma)->s.len++] = (__VA_ARGS__), (ma)->len++, 1))

/**
 * ma_pushmany() - append elements
 * @ma: typed pointer to the mutable array
 * @data: pointer to the elements
 * @dlen: number of elements to insert
 *
 * Return: When successful pointer to the first element inserted, or otherwise
 *	NULL if ma_grow() failed.
 */
#define ma_pushmany(ma, data, dlen) \
	ma_splice((ma), (ma)->s.len, 0, (data), (dlen))

/**
 * ma_unshift() - insert an element at the beginning
 * @ma: typed pointer to the mutable array
 * @...: value to unshift
 *
 * Return: When successful 1, or otherwise 0 if ma_grow() failed.
 *
 * Note!: @... is like in ma_push().
 */
#define ma_unshift(ma, ...) ma_insert((ma), 0, (__VA_ARGS__))

/**
 * ma_unshiftmany() - insert elements at the beginning
 * @ma: typed pointer to the mutable array
 * @data: pointer to the elements
 * @dlen: number of elements to unshift
 *
 * Return: When successful pointer to @ma's new first element, or otherwise NULL
 *	if ma_grow() failed.
 */
#define ma_unshiftmany(ma, data, dlen) \
	ma_splice((ma), 0, 0, (data), (dlen))

/**
 * ma_pop() - delete the last element
 * @ma: typed pointer to the mutable array
 * @ret: pointer that receives the popped element's value, can be NULL
 *
 * If @ret is NULL, then @ma->dtor() is called for the element to be popped.
 *
 * Return: When successful 1, or otherwise 0 if there were no elements to pop.
 */
#define ma_pop(ma, ret)                                                      \
	((ma)->s.len ?                                                       \
	 ((void *)(ret) != NULL) ?                                           \
	 (*(((void *)(ret) != NULL) ?                                        \
	 (ret) : &(ma)->val) = (ma)->buf[--(ma)->s.len--], (ma)->len--, 1) : \
	 (ma)->s.dtor ? ((ma)->s.dtor(&(ma)->buf[--(ma)->s.len],             \
	 		 (ma)->s.userp), (ma)->len--, 1) :                   \
			((ma)->s.len--, (ma)->len--, 1) : 0)

/**
 * ma_shift() - delete the first element
 * @ma: typed pointer to the mutable array
 * @ret: pointer that receives the shifted element's value, can be NULL
 *
 * If @ret is NULL, then @ma->dtor() is called for the element to be shifted.
 *
 * Return: When successful 1, or otherwise 0 if there were no elements to shift.
 */
#define ma_shift(ma, ret)                                                    \
	((ma)->s.len ?                                                       \
	 ((void *)(ret) != NULL) ?                                           \
	 (*(((void *)(ret) != NULL) ? (ret) : &(ma)->val) = (ma)->buf[0],    \
	  memmove(&(ma)->buf[0], &(ma)->buf[1], --(ma)->s.len * (ma)->s.sz), \
	  (ma)->len--, 1) :                                                  \
	 (ma)->s.dtor ? ((ma)->s.dtor(&(ma)->buf[0], (ma)->s.userp),         \
			 memmove(&(ma)->buf[0], &(ma)->buf[1],               \
				 --(ma)->s.len * (ma)->s.sz),                \
			 (ma)->len--, 1) :                                   \
			(memmove(&(ma)->buf[0], &(ma)->buf[1],               \
				 --(ma)->s.len * (ma)->s.sz),                \
			 (ma)->len--, 1) : 0)

/**
 * ma_spawn() - create a new element
 * @ma: typed pointer to the mutable array
 * @offset: position where to spawn
 *
 * Spawn a new element at @offset by either calling @ma->ctor() on it or by
 * setting it to @ma's default value.
 *
 * Return: When successful 1, or otherwise 0 if ma_grow() failed.
 */
#define ma_spawn(ma, offset)                                      \
	(ma_insertp((ma), (offset)) ?                             \
	 ((ma)->s.ctor ? (ma)->s.ctor((ma)->ptr, (ma)->s.userp) : \
	 	       (void)(*(ma)->ptr = (ma)->val), 1) : 0)

/**
 * ma_extract() - extract an element
 * @ma: typed pointer to the mutable array
 * @offset: position where to extract
 * @ret: pointer that receives the extracted element's value
 *
 * Delete the element at @offset without calling @ma->dtor().
 *
 * Return: When successful 1, or otherwise 0 if there was no element to extract.
 */
#define ma_extract(ma, offset, ret)                                           \
	((ma)->s.len ?                                                        \
	 ((ma)->ptr = &(ma)->buf[ma_i2pos((offset), (ma)->s.len)],            \
	  *(ret) = *(ma)->ptr,                                                \
	  memmove((ma)->ptr, (ma)->ptr + 1,                                   \
		  (&(ma)->buf[--(ma)->s.len] - (ma)->ptr) * (ma)->s.sz), 1) : 0)

/**
 * ma_copy() - copy elements into an array
 * @ma: typed pointer to the mutable array
 * @offset: position where the copies are inserted
 * @data: pointer to the elements
 * @dlen: number of elements to copy
 * @callb: like @ma->cmp() but with the first parameter being non const
 *
 * @callb is called with a pointer to the new element that receives the copy and
 * a pointer to the element in @data that is to be copied. It should return 1 if
 * it successfully copied an element or otherwise 0 to skip it.
 *
 * Return: Number of elements copied, or otherwise -1 if ma_grow() failed.
 */
#define ma_copy(ma, offset, data, dlen, callb) \
	(ma_copy)(&(ma)->s, (offset), (data), (dlen), (callb))

/**
 * ma_index() - get the first occurrence of an item
 * @ma: typed pointer to the mutable array
 * @start: position to start searching from
 * @data: pointer to the data to search for
 *
 * Return: Index of the item, or -1 if not found.
 */
#define ma_index(ma, start, data) (ma_index)(&(ma)->s, (start), (data))

/**
 * ma_rindex() - get the last occurrence of an item
 * @ma: typed pointer to the mutable array
 * @start: position to start searching from
 * @data: pointer to the data to search for
 *
 * Return: Index of the item, or -1 if not found.
 */
#define ma_rindex(ma, start, data) (ma_index)(&(ma)->s, (start), (data))

/**
 * ma_search() - search binarily in a sorted array
 * @ma: typed pointer to the sorted mutable array
 * @start: position to start searching from
 * @data: pointer to the data to search for
 *
 * Return: Index of the item, or -1 if not found or @ma->cmp is NULL.
 */
#define ma_search(ma, start, data) (ma_search)(&(ma)->s, (start), (data))

/**
 * ma_sort() - sort all elements
 * @ma: typed pointer to the mutable array
 *
 * Return: When successful 1, or otherwise 0 if @ma->len is 0 or @ma->cmp is
 *	NULL.
 */
#define ma_sort(ma)                     \
	(((ma)->s.len && (ma)->s.cmp) ? \
	 (qsort((ma)->s.buf, (ma)->s.len, (ma)->s.sz, (ma)->s.cmp), 1) : 0)

/**
 * ma_xchg() - exchange two elements
 * @ma: typed pointer to the mutable array
 * @a: position of the first element
 * @b: position of the second element
 *
 * Return: When successful 1, or otherwise 0 if realloc() failed.
 */
#define ma_xchg(ma, a, b) (ma_xchg)(&(ma)->s, (a), (b))

/**
 * ma_reverse() - reverse an array
 * @ma: typed pointer to the mutable array
 *
 * Return: When successful 1, or otherwise 0 if realloc() failed.
 */

#define ma_reverse(ma) (ma_reverse)(&(ma)->s)

/**
 * ma_join() - join all elements into a string
 * @ma: typed pointer to the mutable array
 * @ret: pointer receiving a pointer to the new string
 * @sep: pointer to the null-terminated seperator
 *
 * Return: When successful length of @ret, or otherwise -1 with *@ret NULL if
 *	realloc() failed. You have to free() *@ret, when you no longer
 *	need it.
 */
#define ma_join(ma, ret, sep) (ma_join)(&(ma)->s, (ret), (sep))

/**
 * ma_tostr() - join all elements into a list
 * @ma: typed pointer to the mutable array
 * @ret: pointer receiving a pointer to the new string
 *
 * Return: Same as ma_join().
 */
#define ma_tostr(ma, ret) (ma_join)(&(ma)->s, (ret), ", ")

/* return `offset` as a position between 0 and `max` */
static inline size_t ma_i2pos(int offset, size_t max)
{
	return ((size_t)abs(offset) > max) ?
		(offset > 0) ? max : 0 :
		(offset >= 0) ? (size_t)offset : max + (size_t)offset;
}

/* free an array's buffer */
static inline void ma_freebuf(struct mutarrb *ma)
{
	if (ma->len && ma->dtor) {
		ma_callb dtor = ma->dtor;
		char *elem = ma->buf;
		void *userp = ma->userp;
		size_t i;

		for (i = ma->len; i--; elem += ma->sz)
			dtor(elem, userp);
	}
	ma_xfree(ma->buf);
}

/* detach an array's buffer */
static inline void *(ma_detach)(struct mutarrb *ma, size_t *ret)
{
	void *buf;

	(ma_shrinktofit)(ma);
	buf = ma->buf;
	if (ret)
		*ret = ma->len;
	ma->alloc = 0;
	ma->len = 0;
	ma->buf = NULL;
	return buf;
}

/* make an array's buffer grow */
static inline int (ma_grow)(struct mutarrb *ma, size_t extra)
{
	const double factor = 1.5;
	size_t alloc;
	void *buf;

	if (ma->len + extra <= ma->alloc)
		return 1;
	if (ma->alloc * factor < ma->len + extra)
		alloc = ma->len + extra;
	else
		alloc = ma->alloc * factor;
	buf = ma_xrealloc(ma->buf, alloc, ma->sz);
	if (!buf)
		return 0;
	ma->alloc = alloc;
	ma->buf = buf;
	return 1;
}

/* shrink an array's buffer */
static inline int (ma_shrinktofit)(struct mutarrb *ma)
{
	void *buf;

	if (ma->alloc == ma->len)
		return 1;
	if (ma->len) {
		buf = ma_xrealloc(ma->buf, ma->len, ma->sz);
		if (!buf)
			return 0;
	} else {
		ma_xfree(ma->buf);
		buf = NULL;
	}
	ma->alloc = ma->len;
	ma->buf = buf;
	return 1;
}

/* make room for new elements */
static inline void *(ma_insertmanyp)(struct mutarrb *ma, int offset,
				     size_t nlen)
{
	size_t pos;
	char *buf;

	if (!nlen || (ma->len + nlen > ma->alloc && !(ma_grow)(ma, nlen)))
		return NULL;
	pos = ma_i2pos(offset, ma->len);
	buf = (char *)ma->buf + (pos * ma->sz);
	memmove(buf + (nlen * ma->sz), buf, (ma->len - pos) * ma->sz);
	ma->len += nlen;
	return buf;
}

#endif /* MUTARR_H */
