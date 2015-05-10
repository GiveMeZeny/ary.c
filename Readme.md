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

## Installation

Use the makefile to compile a static/shared library or simply drop [mutarr.c](mutarr.c) and [mutarr.h](mutarr.h) into your project.

## Usage

See [mutarr.h](mutarr.h) for more details.

#### Creating

A mutable array can be of any type and is created like this:

```c
    struct mutarr(int) myints;
    struct mutarr(char *) mycharptrs;
    struct mutarr myvoidptrs; /* see below */
    struct mutarr(struct xyz) mystructs;
```

Before you can use or access it, it has to be initialized:

```c
    ma_init(&myarray, 0); /* initial space for 0 elements */
```

After you're finished, release it:

```c
    ma_release(&myarray);
```

This deletes all of its elements and reinitializes the array.

To retrieve the number of elements an array contains:

```c
    size_t length = myarray.len;
```

###### Note

In general, it __isn't__ safe to supply parameters with side effects (exceptions: see `ma_push`, `ma_unshift` and `ma_insert`).

Also, if you want to pass a mutable array between functions, you have to pre-declare its type:

```c
    struct int_mutarr mutarr(int);

    void print_len(struct int_mutarr *ptr)
    {
        printf("%zu\n", ptr->len);
    }

    int main()
    {
        struct int_mutarr myints;

        ma_init(&myints, 0);
        ma_push(&myints, 10);
        print_len(&myints);
        ma_release(&myints);
        return 0;
    }
```

`struct mutarr` is already declared as `struct mutarr(void *)`.

#### Callbacks

You can configure optional callbacks for each mutable array that are called to serve specific purposes. For their prototypes see [mutarr.h](mutarr.h).

  * `ma_setctor(array, callback)`
  * `ma_setdtor(array, callback)`
  * `ma_setuserp(array, ptr)`

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

    ma_init(&mystuff, 1); /* reserve space for one element */
    ma_setctor(&mystuff, init_whatever);
    ma_setdtor(&mystuff, free_whatever);
    ma_setlen(&mystuff, 1); /* actually add one element */
    ma_release(&mystuff);
```

  * `ma_setdefval(array, value)`
  * `ma_setcmp(array, callb)`
  * `ma_settostr(array, callb)`
  * `ma_setcbs(array, ctor, dtor, cmp, tostr)`

#### Related to the buffer size

  * `ma_attach(array, buffer, len, alloc)`
  * `ma_detach(array, size)`
  * `ma_grow(array, extra)`
  * `ma_shrinktofit(array)`
  * `ma_avail(array)`
  * `ma_swap(array1, array2)`
  * `ma_fullswap(array1, array2)`

#### Related to the contents

To access the array buffer itself, use:

```c
    size_t i;

    for (i = 0; i < myints.len; i++) {
        int x = myints.buf[i];

        printf("myints.buf[%zu] = %d\n", i, x);
    }
```

However, when calling an array function and specifying positions, negative offsets select elements from the end of the array.  
Also, if an offset is beyond any bound it is truncated. Offsets are of type `int`, while lengths are of type `size_t`.

  * `ma_first(array)`
  * `ma_last(array)`

```c
    int x = ma_first(&myints);
    ma_last(&myints) = x;
```

However, when calling either one, it is assumed that the array contains at least one element.

  * `ma_setlen(array, length)`
  * `ma_reset(array)`
  * `ma_splice(array, offset, len, data, dlen)`

#### Inserting

  * `ma_insertp(array, offset)`
  * `ma_insertmanyp(array, offset, n)`
  * `ma_pushp(array)`
  * `ma_insert(array, int offset, value)`
  * `ma_push(array, value)`
  * `ma_unshift(array, value)`

```c
    struct whatever stuff = {.x = y};

    ma_insert(&mystuff, stuff);
    ma_unshift(&mystuff, (struct whatever){.x = 10});
    ma_push(&myptrs, strdup("bla"));
```

  * `ma_insertmany(array, offset, data, dlen)`
  * `ma_pushmany(array, data, dlen)`
  * `ma_unshiftmany(array, data, dlen)`
  * `ma_spawn(array, offset)`
  * `ma_copy(array, offset, data, dlen, callb)`

#### Deleting

  * `ma_delete(array, offset)`
  * `ma_deletemany(array, offset, len)`
  * `ma_pop(array, ret)`
  * `ma_shift(array, ret)`
  * `ma_extract(array, offset, ret)`

```c
    struct whatever stuff;

    ma_pop(&mystuff, NULL);
    ma_extract(&myptrs, 10, &ptr);
    ma_shift(&myints, NULL);
```

## Searching and sorting

  * `ma_index(array, start, data)`
  * `ma_rindex(array, start, data)`
  * `ma_insertsorted(array, data)`
  * `ma_search(array, start, data)`
  * `ma_sort(array)`

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

  * `ma_xchg(array, offset1, offset2)`
  * `ma_reverse(array)`
  * `ma_join(array, ret, sep)`
  * `ma_tostr(array, ret)`

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

```c
    tr_setrealloc(xrealloc); /* your `realloc` has to look like `calloc`, with 3 parameters */
    tr_setfree(xfree);
```

This redirects further calls for any mutable array. Your `realloc()` replacement could e.g. just `exit(-1)` like GNU's `xmalloc()` if there is no memory left, so you spare checking every function that uses dynamic memory wether they succeeded, if you wouldn't react differently anyways.

## License

See [LICENSE](LICENSE).
