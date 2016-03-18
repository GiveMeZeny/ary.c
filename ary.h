#ifndef ARY_H
#define ARY_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define ARY_GROWTH_FACTOR 2.0

/* construct/destruct the element pointed to by `buf` */
typedef void (*ary_elemcb_t)(void *buf, void *userp);

/* the same as the `qsort` comparison function */
typedef int (*ary_cmpcb_t)(const void *a, const void *b);

/* return a malloc()ed string of `buf` in `ret` and its size, or -1 */
typedef int (*ary_joincb_t)(char **ret, const void *buf);

typedef void *(*ary_xalloc_t)(void *ptr, size_t nmemb, size_t size);
typedef void (*ary_xdealloc_t)(void *ptr);

/* struct size: 6x pointers + 4x size_t's + 1x type */
#define ary(type)                                       \
	{                                               \
		struct aryb s;                          \
		size_t len;    /* number of elements */ \
		type *buf;     /* array buffer */       \
		type *ptr;                              \
		type val;                               \
	}

struct aryb {
	size_t len;
	size_t alloc;
	size_t sz;
	void *buf;
	ary_elemcb_t ctor;
	ary_elemcb_t dtor;
	void *userp;
};

/* `struct ary a` is a void *-array */
struct ary ary(void *);
/* `struct ary_xyz a` is a xyz-array... */
struct ary_int ary(int);
struct ary_long ary(long);
struct ary_vlong ary(long long);
struct ary_size_t ary(size_t);
struct ary_double ary(double);
struct ary_char ary(char);
struct ary_charptr ary(char *);

/* predefined callbacks */
void ary_cb_freevoidptr(void *buf, void *userp);
void ary_cb_freecharptr(void *buf, void *userp);

int ary_cb_cmpint(const void *a, const void *b);
int ary_cb_cmplong(const void *a, const void *b);
int ary_cb_cmpvlong(const void *a, const void *b);
int ary_cb_cmpsize_t(const void *a, const void *b);
int ary_cb_cmpdouble(const void *a, const void *b);
int ary_cb_cmpchar(const void *a, const void *b);
int ary_cb_strcmp(const void *a, const void *b);
int ary_cb_strcasecmp(const void *a, const void *b);

int ary_cb_voidptrtostr(char **ret, const void *elem);
int ary_cb_inttostr(char **ret, const void *elem);
int ary_cb_longtostr(char **ret, const void *elem);
int ary_cb_vlongtostr(char **ret, const void *elem);
int ary_cb_size_ttostr(char **ret, const void *elem);
int ary_cb_doubletostr(char **ret, const void *elem);
int ary_cb_chartostr(char **ret, const void *elem);

/* forward declarations */
void ary_freebuf(struct aryb *ary);
void *ary_detach(struct aryb *ary, size_t *ret);
int ary_shrinktofit(struct aryb *ary);
void *ary_splicep(struct aryb *ary, size_t pos, size_t rlen, size_t alen);
int ary_index(struct aryb *ary, size_t *ret, size_t start, const void *data,
              ary_cmpcb_t comp);
int ary_rindex(struct aryb *ary, size_t *ret, size_t start, const void *data,
               ary_cmpcb_t comp);
int ary_reverse(struct aryb *ary);
int ary_join(struct aryb *ary, char **ret, const char *sep,
             ary_joincb_t stringify);
int ary_swap(struct aryb *ary, size_t a, size_t b);
int ary_search(struct aryb *ary, size_t *ret, size_t start, const void *data,
               ary_cmpcb_t comp);

extern ary_xalloc_t ary_xrealloc;

/**
 * ary_use_as_realloc() - set a custom allocator function
 * @routine: replacement for realloc(ptr, nmemb, size)
 */
void ary_use_as_realloc(ary_xalloc_t routine);

/**
 * ary_use_as_free() - set a custom deallocator function
 * @routine: replacement for free(ptr)
 */
void ary_use_as_free(ary_xdealloc_t routine);

/**
 * ary_init() - initialize an array
 * @ary: typed pointer to the array
 * @hint: count of elements to allocate memory for
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed. Always returns
 *	1 if @hint is 0.
 *
 * Note!: Do not directly access elements that are not added to the array yet,
 *	only those with an index below @ary->len.
 */
#define ary_init(ary, hint)                                 \
	((ary)->s.alloc = (ary)->s.len = (ary)->len = 0,    \
	 (ary)->s.sz = sizeof(*(ary)->buf),                 \
	 (ary)->s.ctor = (ary)->s.dtor = NULL,              \
	 (ary)->s.buf = (ary)->s.userp = (ary)->buf = NULL, \
	 ary_grow((ary), (hint)))

/**
 * ary_release() - release an array
 * @ary: typed pointer to the initialized array
 *
 * All elements are removed, the buffer is released and @ary is reinitialized
 * with `ary_init(@ary, 0)`.
 */
#define ary_release(ary)                  \
	do {                              \
		ary_freebuf(&(ary)->s);   \
		(void)ary_init((ary), 0); \
	} while (0)

/**
 * ary_setcbs() - set an array's constructor and destructor
 * @ary: typed pointer to the initialized array
 * @_ctor: routine that creates new elements
 * @_dtor: routine that removes elements
 */
#define ary_setcbs(ary, _ctor, _dtor) \
	((ary)->s.ctor = (_ctor), (ary)->s.dtor = (_dtor), (void)0)

 /**
 * ary_setuserp() - set an array's user-pointer for the ctor/dtor
 * @ary: typed pointer to the initialized array
 * @ptr: pointer that gets passed to the callbacks
 */
#define ary_setuserp(ary, ptr) \
	((ary)->s.userp = (ptr), (void)0)

/**
 * ary_setinitval() - set an array's value used to initialize new elements
 * @ary: typed pointer to the initialized array
 * @...: value that is used for new elements if @ary->ctor() is NULL
 *
 * Note!: @... is like in ary_push(). Also the init-value is left uninitialized
 *	when using ary_init(). However, it has to be specified when
 *	initializing an array with ARY_INIT().
 */
#define ary_setinitval(ary, ...) \
	((ary)->val = (__VA_ARGS__), (void)0)

/**
 * ary_attach() - attach a buffer to an array
 * @ary: typed pointer to the initialized array
 * @nbuf: pointer to a malloc()ed buffer
 * @nlen: number of elements pointed to by @nbuf
 * @nalloc: number of elements the buffer can hold
 *
 * The buffer @nbuf is henceforth owned by @ary and cannot be relied upon
 * anymore and also must not be free()d directly.
 */
#define ary_attach(ary, nbuf, nlen, nalloc)       \
	do {                                      \
		ary_freebuf(&(ary)->s);           \
		(ary)->s.buf = (ary)->buf = nbuf; \
		(ary)->s.len = (ary)->len = nlen; \
		(ary)->s.alloc = nalloc;          \
	} while (0)

/**
 * ary_detach() - detach an array's buffer
 * @ary: typed pointer to the initialized array
 * @size: pointer that receives @ary's length, can be NULL
 *
 * Return: The array buffer of @ary. If @ary's has no allocated memory, NULL is
 *	returned. You have to free() the buffer, when you no longer need it.
 */
#define ary_detach(ary, size)                                             \
	((ary)->ptr = (ary_detach)(&(ary)->s, (size)), (ary)->buf = NULL, \
	 (ary)->len = 0, (ary)->ptr)

/**
 * ary_grow() - allocate new memory in an array
 * @ary: typed pointer to the initialized array
 * @extra: count of extra elements
 *
 * Ensure that @ary can hold at least @extra more elements.
 *
 * Return: When successful 1, otherwise 0 if realloc() failed.
 */
#define ary_grow(ary, extra) \
	((ary_grow)(&(ary)->s, (extra)) ? ((ary)->buf = (ary)->s.buf, 1) : 0)

/**
 * ary_shrinktofit() - release unused allocated memory in an array
 * @ary: typed pointer to the initialized array
 *
 * Return: When successful 1, otherwise 0 if realloc() failed. The array remains
 *	valid in either case.
 */
#define ary_shrinktofit(ary) \
	((ary_shrinktofit)(&(ary)->s) ? ((ary)->buf = (ary)->s.buf, 1) : 0)

/**
 * ary_avail() - get the amount of unused memory in an array
 * @ary: typed pointer to the initialized array
 *
 * Return: The number of elements that can be added without reallocation.
 */
#define ary_avail(ary) \
	((ary)->s.alloc - (ary)->s.len)

/**
 * ary_setlen() - set an array's length
 * @ary: typed pointer to the initialized array
 * @nlen: new number of elements @ary holds
 *
 * If @nlen is above @ary's current length, new elements are added, either by
 * calling @ary->ctor() on them or by using the array's (possibly uninitialized)
 * init-value. Respectively, if @nlen is below @ary's current length,
 * @ary->dtor() is called on all elements above the new length.
 * However, the array is never reallocated and @nlen is truncated to not exceed
 * `@ary.len + ary_avail(@ary)`.
 */
#define ary_setlen(ary, nlen)                                                  \
	do {                                                                   \
		size_t len = (nlen), i;                                        \
		if ((ary)->s.len < len) {                                      \
			if ((ary)->s.alloc < len)                              \
				len = (ary)->s.alloc;                          \
			if ((ary)->s.ctor) {                                   \
				for (i = (ary)->s.len; i < len; i++)           \
					(ary)->s.ctor(&(ary)->buf[i],          \
					              (ary)->s.userp);         \
			} else {                                               \
				for (i = (ary)->s.len; i < len; i++)           \
					(ary)->buf[i] = (ary)->val;            \
			}                                                      \
		} else if ((ary)->s.len > len && (ary)->s.dtor) {              \
			for (i = len; i < (ary)->s.len; i++)                   \
				(ary)->s.dtor(&(ary)->buf[i], (ary)->s.userp); \
		}                                                              \
		(ary)->s.len = (ary)->len = len;                               \
	} while (0)

/**
 * ary_clear() - empty an array
 * @ary: typed pointer to the initialized array
 */
#define ary_clear(ary) \
	ary_setlen((ary), 0)

/**
 * ary_push() - add a new element to the end of an array
 * @ary: typed pointer to the initialized array
 * @...: value to push
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed.
 *
 * Note!: @... is only a single value, it's denoted as varargs in order to cope
 *	with struct-literals, additionally, it is not evaluated if ary_push()
 *	fails (so e.g. `strdup(s)` has no effect on failure).
 */
#define ary_push(ary, ...)                                                   \
	(((ary)->s.len == (ary)->s.alloc) ?                                  \
	 ary_grow((ary), 1) ?                                                \
	 ((ary)->buf[(ary)->s.len++] = (__VA_ARGS__), (ary)->len++, 1) : 0 : \
	 ((ary)->buf[(ary)->s.len++] = (__VA_ARGS__), (ary)->len++, 1))

/**
 * ary_pushp() - add a new element slot to the end of an array (pointer)
 * @ary: typed pointer to the initialized array
 *
 * Return: When successful a pointer to the new element slot, otherwise NULL if
 *	ary_grow() failed.
 */
#define ary_pushp(ary)                                                    \
	(((ary)->s.len == (ary)->s.alloc) ?                               \
	 ary_grow((ary), 1) ? &(ary)->buf[(ary)->s.len++, (ary)->len++] : \
	 NULL : &(ary)->buf[(ary)->s.len++, (ary)->len++])

/**
 * ary_pop() - remove the last element of an array
 * @ary: typed pointer to the initialized array
 * @ret: pointer that receives the popped element's value, can be NULL
 *
 * If @ret is NULL, @ary->dtor() is called for the element to be popped.
 *
 * Return: When successful 1, otherwise 0 if there were no elements to pop.
 */
#define ary_pop(ary, ret)                                            \
	((ary)->s.len ?                                              \
	 ((void *)(ret) != NULL) ?                                   \
	 (*(((void *)(ret) != NULL) ?                                \
	  (ret) : &(ary)->val) = (ary)->buf[--(ary)->s.len],         \
	  (ary)->len--, 1) :                                         \
	 (ary)->s.dtor ? ((ary)->s.dtor(&(ary)->buf[--(ary)->s.len], \
	                                (ary)->s.userp),             \
	                  (ary)->len--, 1) :                         \
	                 ((ary)->s.len--, (ary)->len--, 1) : 0)

/**
 * ary_shift() - remove the first element of an array
 * @ary: typed pointer to the initialized array
 * @ret: pointer that receives the shifted element's value, can be NULL
 *
 * If @ret is NULL, @ary->dtor() is called for the element to be shifted.
 *
 * Return: When successful 1, otherwise 0 if there were no elements to shift.
 */
#define ary_shift(ary, ret)                                                 \
	((ary)->s.len ?                                                     \
	 ((void *)(ret) != NULL) ?                                          \
	 (*(((void *)(ret) != NULL) ? (ret) : &(ary)->val) = (ary)->buf[0], \
	  memmove(&(ary)->buf[0], &(ary)->buf[1],                           \
	          --(ary)->s.len * (ary)->s.sz),                            \
	  (ary)->len--, 1) :                                                \
	 (ary)->s.dtor ? ((ary)->s.dtor(&(ary)->buf[0], (ary)->s.userp),    \
	                 memmove(&(ary)->buf[0], &(ary)->buf[1],            \
	                         --(ary)->s.len * (ary)->s.sz),             \
	                 (ary)->len--, 1) :                                 \
	                (memmove(&(ary)->buf[0], &(ary)->buf[1],            \
	                         --(ary)->s.len * (ary)->s.sz),             \
	                 (ary)->len--, 1) : 0)

/**
 * ary_unshift() - add a new element to the beginning of an array
 * @ary: typed pointer to the initialized array
 * @...: value to unshift
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed.
 *
 * Note!: @... is like in ary_push().
 */
#define ary_unshift(ary, ...) \
	(ary_unshiftp(ary) ? (*(ary)->ptr = (__VA_ARGS__), 1) : 0)

/**
 * ary_unshiftp() - add a new element slot to the beginning of an array
 * @ary: typed pointer to the initialized array
 *
 * Return: When successful a pointer to the new element slot, otherwise NULL if
 *	ary_grow() failed.
 */
#define ary_unshiftp(ary) \
	ary_splicep((ary), 0, 0, 1)

/**
 * ary_splice() - add/remove elements from an array
 * @ary: typed pointer to the initialized array
 * @pos: index at which to add/remove
 * @rlen: number of elements to remove
 * @data: pointer to new elements
 * @dlen: number of new elements to add
 *
 * Return: When successful 1, otherwise 0 if there were new elements to add but
 *	ary_grow() failed (the array remains unchanged in this case).
 */
#define ary_splice(ary, pos, rlen, data, dlen)                      \
	(ary_splicep((ary), (pos), (rlen), (dlen)) ?                \
	 (memcpy((ary)->ptr, (data), (dlen) * (ary)->s.sz), 1) : 0)

/**
 * ary_splicep() - add element slots/remove elements from an array
 * @ary: typed pointer to the initialized array
 * @pos: index at which to add/remove
 * @rlen: number of elements to remove
 * @alen: number of new element slots to add
 *
 * Return: When successful a pointer to the first new element slot (position of
 *	the last removed element), otherwise NULL if there were new elements
 *	slots to allocate but ary_grow() failed (the array remains unchanged in
 *	this case).
 */
#define ary_splicep(ary, pos, rlen, alen)                                     \
	(((ary)->ptr = (ary_splicep)(&(ary)->s, (pos), (rlen), (alen))) ?     \
	 ((ary)->buf = (ary)->s.buf, (ary)->len = (ary)->s.len, (ary)->ptr) : \
	 NULL)

/**
 * ary_index() - get the first occurrence of an element in an array
 * @ary: typed pointer to the initialized array
 * @ret: pointer that receives the element's position, can be NULL
 * @start: position to start looking from
 * @data: pointer to the data to look for
 * @comp: comparison function, if NULL then memcmp() is used
 *
 * Return: When successful 1 and @ret is set to the position of the element
 *	found, otherwise 0 and @ret is uninitialized.
 */
#define ary_index(ary, ret, start, data, comp)                       \
	((ary)->ptr = (data), (ary_index)(&(ary)->s, (ret), (start), \
	                                  (ary)->ptr, (comp)))

/**
 * ary_rindex() - get the last occurrence of an element in an array
 * @ary: typed pointer to the initialized array
 * @ret: pointer that receives the element's position, can be NULL
 * @start: position to start looking from (backwards)
 * @data: pointer to the data to look for
 * @comp: comparison function, if NULL then memcmp() is used
 *
 * Return: When successful 1 and @ret is set to the position of the element
 *	found, otherwise 0 and @ret is uninitialized.
 */
#define ary_rindex(ary, ret, start, data, comp)                       \
	((ary)->ptr = (data), (ary_rindex)(&(ary)->s, (ret), (start), \
	                                   (ary)->ptr, (comp)))

/**
 * ary_reverse() - reverse an array
 * @ary: typed pointer to the initialized array
 *
 * Return: When successful 1, otherwise 0 if realloc() failed.
 */

#define ary_reverse(ary) \
	(ary_reverse)(&(ary)->s)

 /**
 * ary_sort() - sort all elements in an array
 * @ary: typed pointer to the initialized array
 * @comp: comparison function
 */
#define ary_sort(ary, comp) \
	qsort((ary)->s.buf, (ary)->s.len, (ary)->s.sz, (comp))

/**
 * ary_join() - join all elements of an array into a string
 * @ary: typed pointer to the initialized array
 * @ret: pointer that receives a pointer to the new string
 * @sep: pointer to the null-terminated separator
 * @stringify: stringify function, if NULL then @ary is assumed to be a char *-
 *	array
 *
 * Return: When successful length of @ret, otherwise -1 with `*@ret == NULL` if
 *	realloc() failed. You have to free() *@ret, when you no longer need it.
 */
#define ary_join(ary, ret, sep, stringify) \
	(ary_join)(&(ary)->s, (ret), (sep), (stringify))

/**
 * ary_slice() - select a part of an array into a new one
 * @ary: typed pointer to the initialized array
 * @ret: typed pointer to an unitialized array
 * @start: position where to start the selection
 * @end: position where to end the selection (excluding)
 *
 * @ret will contain a shallow copy of the selected elements and is always
 * initialized with `ary_init(@ret, 0)`. @ary's init-value is also copied.
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed.
 */
#define ary_slice(ary, ret, start, end)                                   \
	((void)ary_init((ret), 0),                                        \
	 ary_splice((ret), 0, 0, &(ary)->buf[(start)],                    \
	            ((start) < (ary)->s.len) ? ((start) < (end)) ?        \
	                                       (end) - (start) : 0 : 0) ? \
	 ((ret)->val = (ary)->val, 1) : 0)

/**
 * ary_clone() - create a shallow copy of an array
 * @ary: typed pointer to the initialized array
 * @ret: typed pointer to an unitialized array
 *
 * Same as `ary_slice(&old, &new, 0, old.len)`.
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed.
 */
#define ary_clone(ary, ret) \
	ary_slice((ary), (ret), 0, (ary)->s.len)

/**
 * ary_insert() - add a new element to an array at a given position
 * @ary: typed pointer to the initialized array
 * @pos: position where to insert
 * @...: value to insert
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed.
 *
 * Note!: @... is like in ary_push().
 */
#define ary_insert(ary, pos, ...)              \
	((ary_splicep((ary), (pos), 0, 1)) ?   \
	 (*(ary)->ptr = (__VA_ARGS__), 1) : 0)

/**
 * ary_insertp() - add a new element slot to an array at a given position
 * @ary: typed pointer to the initialized array
 * @pos: position where to insert
 *
 * Return: When successful a pointer to the new element slot, otherwise NULL if
 *	ary_grow() failed.
 */
#define ary_insertp(ary, pos) \
	ary_splicep((ary), (pos), 0, 1)

/**
 * ary_remove() - remove an element of an array
 * @ary: typed pointer to the initialized array
 * @pos: position of the element to remove
 *
 * Return: Pointer to the element following the deleted element.
 */
#define ary_remove(ary, pos) \
	ary_splicep((ary), (pos), 1, 0)

/**
 * ary_emplace() - create a new element in an array
 * @ary: typed pointer to the initialized array
 * @pos: position where to create the element
 *
 * After allocating the new element, it is initialized by either calling
 * @ary->ctor() on it or by using the array's (possibly uninitialized) init-
 * value.
 *
 * Return: When successful 1, otherwise 0 if ary_grow() failed.
 */
#define ary_emplace(ary, pos)                                         \
	(ary_insertp((ary), (pos)) ?                                  \
	 ((ary)->s.ctor ? (ary)->s.ctor((ary)->ptr, (ary)->s.userp) : \
	                  (void)(*(ary)->ptr = (ary)->val), 1) : 0)

/**
 * ary_snatch() - remove an element of an array without calling the destructor
 * @ary: typed pointer to the initialized array
 * @pos: position of the element to remove
 * @ret: pointer that receives the removed element's value, can be NULL
 *
 * Return: When successful 1, otherwise 0 if there was no element to remove.
 */
#define ary_snatch(ary, pos, ret)                                         \
	((ary)->s.len ?                                                   \
	 ((void *)(ret) != NULL) ?                                        \
	 ((ary)->ptr = &(ary)->buf[((pos) < (ary)->s.len) ?               \
	                           (pos) : (ary)->s.len - 1],             \
	  *(((void *)(ret) != NULL) ? (ret) : &(ary)->val) = *(ary)->ptr, \
	  memmove((ary)->ptr, (ary)->ptr + 1,                             \
	          &(ary)->buf[--(ary)->s.len] - (ary)->ptr),              \
	  (ary)->len--, 1) :                                              \
	 ((ary)->ptr = &(ary)->buf[((pos) < (ary)->s.len) ?               \
	                           (pos) : (ary)->s.len - 1],             \
	  memmove((ary)->ptr, (ary)->ptr + 1,                             \
	          &(ary)->buf[--(ary)->s.len] - (ary)->ptr),              \
	  (ary)->len--, 1) : 0)                                           \

/**
 * ary_swap() - swap two elements in an array
 * @ary: typed pointer to the initialized array
 * @a: position of the first element
 * @b: position of the second element
 *
 * Return: When successful 1, otherwise 0 if realloc() failed.
 */
#define ary_swap(ary, a, b) \
	(ary_swap)(&(ary)->s, (a), (b))

/**
 * ary_search() - search a sorted array for an element
 * @ary: typed pointer to the sorted array
 * @ret: pointer that receives the element's position, can be NULL
 * @start: position to start searching from
 * @data: pointer to the data to search for
 * @comp: comparison function
 *
 * Return: When successful 1 and @ret is set to the position of the element
 *	found, otherwise 0 and @ret is uninitialized.
 */
#define ary_search(ary, ret, start, data, comp)                       \
	((ary)->ptr = (data), (ary_search)(&(ary)->s, (ret), (start), \
	                                   (ary)->ptr, (comp)))

static inline int (ary_grow)(struct aryb *ary, size_t extra)
{
	const double factor = ARY_GROWTH_FACTOR;
	size_t alloc;
	void *buf;

	if (ary->len + extra <= ary->alloc)
		return 1;
	if (ary->alloc * factor < ary->len + extra)
		alloc = ary->len + extra;
	else
		alloc = ary->alloc * factor;
	buf = ary_xrealloc(ary->buf, alloc, ary->sz);
	if (!buf)
		return 0;
	ary->alloc = alloc;
	ary->buf = buf;
	return 1;
}

#endif /* ARY_H */
