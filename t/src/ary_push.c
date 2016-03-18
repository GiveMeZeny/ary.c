#include "tap.h"
#include "ary.h"

struct ary(int) a;

int main()
{
	ary_init(&a, 0);

	ok(ary_push(&a, 10), "Pushed 10 to Array");
	is(a.len, (size_t)1, "%zu", "It now has 1 element");

	ok(ary_push(&a, 20), "Pushed 20 to Array");
	ok(ary_push(&a, 30), "Pushed 30 to Array");
	is(a.len, (size_t)3, "%zu", "It now has 3 elements");

	is(a.buf[0], 10, "%d", "1. element is 10");
	is(a.buf[1], 20, "%d", "2. element is 20");
	is(a.buf[2], 30, "%d", "3. element is 30");

	ary_release(&a);

	done_testing();
}
