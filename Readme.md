ary.c
=====
A type-safe and generic C99-compliant dynamic array.

## Example

```c
    struct ary_double numbers = ARY_INIT(7.0);
    double ret;

    ary_push(&numbers, 1.0);
    ary_insert(&numbers, 1, 2.0);
    ary_emplace(&numbers, numbers.len);
    ary_pop(&numbers, &ret);
    printf("%g\n", ret);
    ary_release(&numbers);
```

## Installation

Invoke `make` to compile a static library or simply drop [ary.c](ary.c) and [ary.h](ary.h) into your project.

## Usage

See [ary.h](ary.h) for more details.

  * It's __not__ safe to pass parameters that have side effects like `i++` or whatever, because often parameters are evaluated more than once (exceptions: see `ary_push()`, `ary_unshift()` and `ary_insert()`).
  * If a function receives an index that exceeds the maximum possible position, it's truncated.

#### Creating

The array can be of any type and is created like this:

```c
    struct ary(short) myshorts;
    struct ary_int myints;
    struct ary(int *) myintptrs;
    struct ary_charptr mycharptrs;
    struct ary myvoidptrs;
    struct ary(struct xyz) mystructs;
```

If you want to pass an array between functions, you have to pre-declare its type:

```c
    struct ary_Pos ary(struct Pos);

    void print_len(struct ary_Pos *ptr)
    {
        printf("%zu\n", ptr->len);
    }

    int main()
    {
        struct ary_Pos a = ARY_INIT((struct Pos){.x=0,.y=0});

        ary_push(&a, (struct Pos){.x=1,.y=1});
        print_len(&a);
        ary_release(&a);
        return 0;
    }
```

These are already pre-declared:

```c
    struct ary ary(void *);
    struct ary_int ary(int);
    struct ary_long ary(long);
    struct ary_vlong ary(long long);
    struct ary_size_t ary(size_t);
    struct ary_double ary(double);
    struct ary_char ary(char);
    struct ary_charptr ary(char *);
```

Before you can use or access an array, it has to be initialized:

```c
    ary_init(&a, x); /* already allocate memory for x elements to reduce further reallocations */
```

Or directly when defining it:

```c
    struct ary_size_t a = ARY_INIT((size_t)0); /* new elements get set to 0 (must be of the exact same type!) */
```

After you're finished, release it:

```c
    ary_release(&a);
```

This removes all of its elements, releases the allocated memory and reinitializes the array.

#### `typedef` Syntax

Instead of the way from above:

```c
    struct ary_Pos ary(struct Pos);
    
    struct ary(struct Pos) mypositions1;
    struct ary_Pos mypositions2;
```

you can also declare arrays like this:

```c
    typedef Array(struct Pos) Array_Pos;
    
    Array(struct Pos) mypositions1;
    Array_Pos mypositions2;
```

To disable this syntax, use `#define ARY_STRUCT_ONLY` or `-DARY_STRUCT_ONLY`.

#### Callbacks

You can set optional constructors, destructors and user-pointers that get passed along. The constructor is called for new elements that get added via `ary_setlen()` or `ary_emplace()`. If it's _NULL_, those elements are initialized with the array's _init-value_ (which is still uninitialized when using `ary_init()` for initializing the array). The destructor is used for elements that are to be removed via `ary_setlen()`, `ary_pop()`, `ary_shift()`, `ary_clear()`. For their prototypes see [ary.h](ary.h).

  * `ary_setcbs(array, ctor, dtor)`
  * `ary_setuserp(array, ptr)`
  * `ary_setinitval(array, value)`

```c
    void init_bla(void *buf, void *userp)
    {
        struct bla *s = buf;

        (void)userp; /* user-pointer is neither used nor set */
        s->xyz = malloc(10);
    }

    void free_bla(void *buf, void *userp)
    {
        struct bla *s = buf;

        (void)userp;
        free(s->xyz);
    }

    struct ary(struct bla) a;

    ary_init(&a, 1); /* reserve space for one element */
    ary_setcbs(&a, init_whatever, free_whatever);
    ary_setlen(&a, 1); /* actually add one element */
    ary_release(&a);
```

  * `ary_index()`, `ary_rindex()`, `ary_sort()` and `ary_search()` expect a comparison-callback
  * `ary_join()` expects a stringify-callback

A couple of such callbacks are already defined like `ary_cb_freevoidptr()`, `ary_cb_freecharptr()`, `ary_cb_cmpint()`, `ary_cb_strcmp()`, `ary_cb_voidptrtostr()`, `ary_cb_inttostr()`, ... (see [ary.c](ary.c)).

#### Related to the buffer

  * `ary_attach(array, buffer, len, alloc)`
  * `ary_detach(array, &size)`
  * `ary_grow(array, extra)`
  * `ary_shrinktofit(array)`
  * `ary_avail(array)`

#### Related to the contents

To access the array's buffer, use:

```c
    size_t length = myints.len;

    for (size_t i = 0; i < length; i++)
        printf("myints.buf[%zu] = %d\n", i, myints.buf[i]);
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

  * `ary_clone(array, newarray)`
  * `ary_insert(array, position, value)`
  * `ary_remove(array, position)`
  * `ary_emplace(array, position)`
  * `ary_snatch(array, position, &ret)`
  * `ary_swap(array, position1, position2)`
  * `ary_search(array, ret, start, data, comp)`

#### Adding new element slots

  * `ary_pushp(array)`
  * `ary_unshiftp(array)`
  * `ary_splicep(array, position, rlen, alen)`
  * `ary_insertp(array, position)`

#### Replacing malloc()

```c
    ary_use_as_realloc(xrealloc); /* ary_* will use xrealloc(ptr, nmemb, size) */
    ary_use_as_free(xfree); /* ary_* will use xfree(ptr) */
```

## License

See [LICENSE](LICENSE).
