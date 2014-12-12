mutarr.c
========
A type-safe and generic C99-compliant dynamic array.

## Example

```c
    struct mutarr(double) numbers;
    double ret;

    ma_init(&numbers, 0);
    ma_push(&numbers, 1.0);
    ma_push(&numbers, 2.0);
    ma_push(&numbers, 3.0);
    if (ma_pop(&numbers, &ret))
        printf("%g\n", ret);
    ma_release(&numbers);
```
See [mutarr.h](mutarr.h) for more details.

## Installation

Use the makefile to compile a static/shared library or simply drop [mutarr.c](mutarr.c) and [mutarr.h](mutarr.h) into your project.

## Usage

#### Creating

A mutable array can be of any type and is created like this:
```c
    struct mutarr(int) myints;
    struct mutarr(char *) mycharps;
    struct mutarr myvoidps; /* see below */
    struct mutarr(struct xyz) mystructs;
```
Before you can use or access it, it has to be initialized:
```c
    ma_init(&myarray, 0); /* initial space for 0 elements */
```
`ma_init()` returns _1_, or _0_ if the initial space is above zero, but allocation failed.

After you're finished, release it:
```c
    ma_release(&myarray);
```
This deletes all of its elements and reinitializes the array.

To retrieve the number of elements an array contains:
```c
    size_t length = myarray.len;
```

###### Note 1

If you want to pass a mutable array to other functions, you have to pre-declare its type (casting to `void *` isn't enough in this case and results in _undefined behaviour_, theoretically):
```c
    struct int_mutarr mutarr(int);

    void fun2(struct int_mutarr *ptr)
    {
        printf("%zu\n", ptr->len);
    }

    int main()
    {
        struct int_mutarr myints;

        ma_init(&myints, 0);
        ma_push(&myints, 10);
        fun2(&myints);
        ma_release(&myints);
        return 0;
    }
```
`struct mutarr` is already declared as `struct mutarr(void *)` for convenience.

###### Note 2

In general, it _isn't_ safe to supply parameters with side effects.

###### Note 3

Negative indices select elements from the end of the array and if an index is beyond any bound it is truncated.

#### Callbacks

You can configure optional callbacks for each mutable array that are called to serve specific purposes. For their prototypes see [mutarr.h](mutarr.h).

##### _ma\_setctor(array, callb)_
Set the array's constructor that is called on buffers of newly created elements.

##### _ma\_setdtor(array, callb)_
Set the array's destructor that is called on buffers of elements that are to be deleted.

##### _ma\_setuserp(array, ptr)_
Set an arbitrary user pointer that is passed to both the con- and destructor.
```c
    void init_whatever(void *buf, void *userp)
    {
        struct whatever *s = buf;

        s->ptr = malloc(10);
    }

    void free_whatever(void *buf, void *userp)
    {
        struct whatever *s = buf;

        free(s->ptr);
    }

    struct mutarr(struct whatever) mystuff;

    ma_init(&mystuff, 1);
    ma_setctor(&mystuff, init_whatever);
    ma_setdtor(&mystuff, free_whatever);
    ma_setlen(&mystuff, 1);
    ma_release(&mystuff);
```

##### _ma\_setdefval(array, value)_
This value is used for initializing newly created elements when no constructor is set.

##### _ma\_setcmp(array, callb)_
This callback is necessary for `ma_insertsorted()`, `ma_sort()`, `ma_search()` and some cases for `ma_index()` and `ma_rindex()`.

##### _ma\_settostr(array, callb)_
This callback is necessary for `ma_join()` and `ma_tostr()`.

##### _ma\_setcbs(array, ctor, dtor, cmp, tostr)_
Set all callbacks at once, `NULL` is valid.

#### Related to the buffer size

##### _ma\_attach(array, buffer, len, alloc)_
Attach a `malloc()`ed buffer to the array. You need to specify the number of elements `buffer` contains and the max number of elements it can hold.

##### _ma\_detach(array, size)_
Detach the buffer from the array. `size` receives the number of elements it contains. If you no longer need the buffer, you have to `free()` it.

##### _ma\_grow(array, extra)_
Ensure that the array can hold at least `extra` more elements. Returns _1_, or _0_ if reallocation is needed but failed. However, this doesn't add any elements but only allocates space for new ones if necessary.

##### _ma\_shrinktofit(array)_
Make the buffer size match the number of elements it contains. May return _0_, if reallocation is needed but failed.

##### _ma\_avail(array)_
Returns the number of elements that can be added without further reallocation.

##### _ma\_swap(array1, array2)_
This swaps two arrays' buffers. Returns _1_ on success, or _0_ if their type sizes mismatch and nothing is swapped.

##### _ma\_fullswap(array1, array2)_
Completely swap two arrays, including their callbacks and default values. Returns the same as `ma_swap()`, but may also return _0_ when allocation failed.

#### Related to the contents

To access the array buffer itself, use:
```c
    size_t i;

    for (i = 0; i < myints.len; i++) {
    	int x = myints.buf[i];

    	printf("myints.buf[%zu] = %d\n", i, x);
    }
```

##### _ma\_first(array)_ and _ma\_last(array)_
Return references to the first and the last element of an array, respectively.
```c
    int x = ma_first(&myints);
    ma_last(&myints) = x;
```

##### _ma\_setlen(array, length)_
Adjust the length of an array. If `length` is less than the current length, all elements above the new length are deleted (see `ma_setdtor()`). If `length` is greater, new elements are created (see `ma_setctor()` and `ma_setdefval()`). `length` cannot be greater than the actual length plus `ma_avail()`.

##### _ma\_reset(array)_
This deletes all elements in `array`.

##### _ma\_splice(array, offset, len, data, dlen)_
Delete all elements between `offset..offset+len`, replace them with them with the given data. Returns a pointer to the element following the last deleted one if `dlen` is zero. Returns a pointer to the first inserted element, or `NULL` if allocation failed, if it isn't.

#### Inserting

##### _ma\_insertp(array, offset)_
##### _ma\_insertmanyp(array, offset, n)_
##### _ma\_pushp(array)_
These functions return a pointer to the first inserted element's buffer or `NULL` if allocation failed.

##### _ma\_insert(array, offset, value)_
##### _ma\_push(array, value)_
##### _ma\_unshift(array, value)_
Insert a value into `array` and return _1_ when successful, or _0_ if allocation failed.
```c
    struct whatever stuff = {.x = y};

    ma_insert(&mystuff, stuff);
    ma_unshift(&mystuff, (struct whatever){.x = 10});
    ma_push(&myptrs, strdup("bla"));
```
If the functions fails, `value` is not evaluated.

##### _ma\_insertmany(array, offset, data, dlen)_
##### _ma\_pushmany(array, data, dlen)_
##### _ma\_unshiftmany(array, data, dlen)_
These functions take `dlen` elements pointed to by `data` and insert them appropiately into `array`. They also return either a pointer to the first inserted element's buffer or `NULL` if allocation failed.

##### _ma\_spawn(array, offset)_
Create a new element at `offset`. This calls the array's constructor or uses its default value to initialize the element.

##### _ma\_copy(array, offset, data, dlen, callb)_
Like `ma_insertmany()`, but uses an additional callback that is called for every element in `data` and saves copies in `array`. Useful when deep copying pointers. Returns the number of elements copied, or _-1_ if reallocation failed.

#### Deleting

##### _ma\_delete(array, offset)_
##### _ma\_deletemany(array, offset, len)_
Delete one or `len` elements at `offset` and use the array's destructor, if set. Returns a pointer to the element following the last deleted one.

##### _ma\_pop(array, ret)_
##### _ma\_shift(array, ret)_
If `ret` isn't `NULL`, `ret` receives the value of the element to be removed from the array. If it is `NULL`, the element is deleted by using the array's destructor if set. If there is no element _0_ is returned.

##### _ma\_extract(array, offset, ret)_
Like `ma_pop()` and `ma_unshift()`, but `ret` cannot be `NULL`. It always receives the extracted element, which is simply removed without using the array's destructor. May return _0_ if there is no element to extract.

```c
    struct whatever stuff;

    ma_pop(&mystuff, NULL);
    ma_extract(&myptrs, 10, &ptr);
    ma_shift(&myints, NULL);
```

## Searching and sorting

##### _ma\_index(array, start, data)_ and _ma\_index(array, start, data)_
Returns the index of the element pointed to by `data` in `array`, or -1 if not found. If there is no compare-callback set, `memcmp()` is used instead.

##### _ma\_insertsorted(array, data)_
Inserts an element into a sorted array. Returns _1_, or _0_ if there is no compare-callback set or reallocation failed.

##### _ma\_search(array, start, data)_
Like `ma_index()` and `ma_rindex()`, but searches in a sorted array. Returns the element's index, or -1 if not found or there is no compare-callback set.

##### _ma\_sort(array)_
Sorts an array with `qsort()`. Returns _1_, or _0_ if `array` is empty or there is no compare-callback set.

```c
    int cmpints(const void *a, const void *b)
    {
        const int *x = a;
        const int *y = b;

        return *x - *y;
    }

    struct mutarr(int) myints;

    ma_init(&myints, 0);
    ma_setcmp(&myints, cmpints);
    ma_insertsorted(&myints, &(int){5});
    ma_insertsorted(&myints, &(int){8});
    ma_insertsorted(&myints, &(int){7});
    ma_push(&myints, 2);
    ma_sort(&myints);
    printf("index: %d\n", ma_search(&myints, 0, &(int){8}));
    ma_release(&myints);
```

#### Misc

##### _ma\_xchg(array, offset1, offset2)_
Exchanges two elements. Returns _1_, or _0_ if allocation failed.

##### _ma\_reverse(array)_
Reverse all elements in `array`. Returns _1_, or _0_ if allocation failed.

##### _ma\_join(array, ret, seperator)_
`ret` receives a pointer to the string representation of `array` that has to be `free()`ed if no longer needed. Returns the length of the string, or -1 with `ret` set to `NULL` uf allocation failed.

##### _ma\_tostr(array, ret)_
Like `ma_join()` with the seperator being `", "`.

```c
    int doubletostr(char **ret, const void *elem)
    {
    	const double *x = elem;

        return asprintf(ret, "%g", *x;
    }

    struct mutarr(double) sevens;
    char *str;

    ma_init(&sevens, 0);
    ma_setdefval(&sevens, 7.1);
    ma_settostr(&sevens, doubletostr);
    ma_grow(&sevens, 2);
    ma_setlen(&sevens, 2);
    sevens.buf[1] = 7.2;
    ma_push(&sevens, 7.3);
    ma_spawn(&sevens, sevens.len);
    ma_xchg(&sevens, 0, 2);
    ma_tostr(&sevens, &str);
    printf("sevens: {%s}\n", str);
    free(str);
    ma_release(&sevens);
```

#### Replacing malloc()

You can use your own allocator functions:
```c
    tr_setrealloc(xrealloc); /* your `realloc` has to look like `calloc`, with 3 parameters
    tr_setfree(xfree);
```
This redirects further calls for any mutable array. Your `realloc()` replacement could e.g. just `exit(-1)` like GNU's `xmalloc()` if there is no memory left, so you spare checking every function that uses dynamic memory wether they succeeded, if you wouldn't react differently anyways.

## License

See [LICENSE](LICENSE).
