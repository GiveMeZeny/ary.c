ary.c
=====
A type-safe and generic C99-compliant dynamic array.

## Example

```c
    struct ary(double) a;
    char *str;
    double ret, tmp;
    size_t pos;

    ary_init(&a, 0);

    ary_push(&a, 10.1);
    ary_push(&a, 20.2);
    ary_push(&a, 10.1);
    ary_splice(&a, 3, 0, ((double[]){10.1, 20.2}), 2);
    ary_insert(&a, 1, 30.3);

    ary_join(&a, &str, ", ", ary_cb_doubletostr);
    printf("original: [%s]\n", str);
    free(str);

    ary_unique(&a, ary_cb_cmpdouble);
    ary_sort(&a, ary_cb_cmpdouble);
    ary_reverse(&a);

    ary_join(&a, &str, ", ", ary_cb_doubletostr);
    printf("sorted & reversed uniques: [%s]\n", str);
    free(str);

    ary_shift(&a, &ret);
    ary_unshift(&a, ret + 0.3);

    a.buf[1] += 0.2;

    ary_join(&a, &str, ", ", ary_cb_doubletostr);
    printf("altered: [%s]\n", str);
    free(str);

    tmp = 10.1;
    ary_index(&a, &pos, 0, &tmp, NULL);
    printf("%g found @ a[%zu]\n", tmp, pos);

    ary_release(&a);

    /* output:
     *   original: [10.1, 30.3, 20.2, 10.1, 10.1, 20.2]
     *   sorted & reversed uniques: [30.3, 20.2, 10.1]
     *   altered: [30.6, 20.4, 10.1]
     *   10.1 found @ a[2]
     */
```

## Installation

Invoke `make` to compile a static library or simply drop [ary.c](ary.c) and [ary.h](ary.h) into your project.

## Usage

Everything's documented in [ary.h](ary.h).

  * It's __not__ safe to pass parameters that have side effects like `i++`, `strcat(ptr, "...")` or whatever, because often parameters are evaluated more than once (exceptions: see `ary_push()`, `ary_unshift()` and `ary_insert()`).
  * If a function receives an index that exceeds the maximum possible position, it is truncated.
  * All length, positions and indices are `size_t`s.

#### Creating

An array can be of any type and is created like this:

```c
    struct ary(void *) myvoidptrs;
    struct ary(int) myints;
    struct ary(char *) mycharptrs;
    struct ary(struct xyz) mystructs;
    struct ary(struct ary(int)) myarys;
```

Before you can use or access an array, it has to be initialized:

```c
    ary_init(&a, x); /* already allocate memory for x elements to reduce further reallocations */
```

After you're finished, release it:

```c
    ary_release(&a);
```

This removes all of its elements, releases the allocated memory and reinitializes the array.

#### Type declaration

If you want to have an array in a function's parameter list, you have to declare its type to keep it std-compliant:

```c
    struct ary_Pos ary(struct Pos);

    void print_len(struct ary_Pos *ptr)
    {
        printf("%zu\n", ptr->len);
    }

    int main()
    {
        struct ary_Pos a;

        ary_init(&a, 0);
        ary_push(&a, (struct Pos){.x=1,.y=1});
        print_len(&a);
        ary_release(&a);
        return 0;
    }
```

These types are already declared:

```c
    struct ary ary(void *);
    struct ary_int ary(int);
    struct ary_long ary(long);
    struct ary_vlong ary(long long);
    struct ary_size_t ary(size_t);
    struct ary_double ary(double);
    struct ary_char ary(char);
    struct ary_charptr ary(char *);

    /* examples from above: */
    struct ary myvoidptrs;
    struct ary_int myints;
    struct ary_charptr mycharptrs;
```

#### Related to the buffer

  * `ary_attach(array, buffer, len, alloc)`
  * `ary_detach(array, &size)`
  * `ary_grow(array, extra)`
  * `ary_shrinktofit(array)`
  * `ary_avail(array)`

#### Related to the contents

To access the array's buffer, use:

```c
    size_t length = a.len;

    for (size_t i = 0; i < length; i++)
        printf("a.buf[%zu] = %d\n", i, a.buf[i]);
```

  * `ary_setlen(array, length)`
  * `ary_clear(array)`

#### Basic functionality

  * `ary_push(array, value)`
  * `ary_pop(array, &ret)`
  * `ary_shift(array, &ret)`
  * `ary_unshift(array, value)`
  * `ary_splice(array, offset, rlen, data, dlen)`
  * `ary_index(array, ret, start, data, comp)`
  * `ary_rindex(array, ret, start, data, comp)`
  * `ary_reverse(array)`
  * `ary_sort(array, comp)`
  * `ary_join(array, ret, sep, stringify)`
  * `ary_slice(array, newarray, start, end)`

#### More

  * `ary_insert(array, position, value)`
  * `ary_remove(array, position)`
  * `ary_emplace(array, position)`
  * `ary_snatch(array, position, &ret)`
  * `ary_clone(array, newarray)`
  * `ary_unique(array, comp)`
  * `ary_swap(array, position1, position2)`
  * `ary_search(array, ret, start, data, comp)`

#### Adding new element slots

  * `ary_pushp(array)`
  * `ary_unshiftp(array)`
  * `ary_splicep(array, position, rlen, alen)`
  * `ary_insertp(array, position)`

#### Callbacks

You can set an optional constructor and an optional destructor. The constructor is called for new elements that were added by `ary_setlen()` and `ary_emplace()`. The destructor is called for elements that are to be removed by `ary_setlen()`, `ary_pop()`, `ary_shift()` and `ary_clear()`. For their prototypes see [ary.h](ary.h).

  * `ary_setcbs(array, ctor, dtor)`
  * `ary_setuserp(array, ptr)`

```c
    void init_foo(void *buf, void *userp)
    {
        struct foo *s = buf;

        (void)userp;
        s->xyz = malloc(10);
    }

    void free_foo(void *buf, void *userp)
    {
        struct foo *s = buf;

        (void)userp;
        free(s->xyz);
    }

    struct ary(struct foo) a;

    ary_init(&a, 0);
    ary_setcbs(&a, init_whatever, free_whatever);
    ary_grow(&a, 5);
    ary_setlen(&a, 5);
    ary_release(&a);
```

  * `ary_setinitval(array, value)`

    If the constructor is _NULL_, new elements are initialized with the array's _init-value_ which should be set via `ary_setinitval()` if needed, otherwise it's a possibly uninitialized value.

  * `ary_index()`, `ary_rindex()`, `ary_sort()` and `ary_search()` expect a comparison-callback
  * `ary_join()` expects a stringify-callback

A couple of such callbacks are already defined like `ary_cb_freevoidptr()`, `ary_cb_freecharptr()`, `ary_cb_cmpint()`, `ary_cb_strcmp()`, `ary_cb_voidptrtostr()`, `ary_cb_longtostr()`, ... (see [ary.c](ary.c)).

#### Replacing malloc()

```c
    ary_use_as_realloc(xreallocarray); /* ary_* will use xreallocarray(ptr, nmemb, size) */
    ary_use_as_free(xfree); /* ary_* will use xfree(ptr) */
```

## License

See [LICENSE](LICENSE).
