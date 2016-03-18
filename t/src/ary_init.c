#include "tap.h"
#include "ary.h"

struct ary(int) a;

int main()
{
	ary_init(&a, 0);

	is(a.len, (size_t)0, "%zu", "Array is empty");
	is(ary_avail(&a), (size_t)0, "%zu", "and has no capacity");

	ary_release(&a);

	ary_init(&a, 120);

	is(a.len, (size_t)0, "%zu", "Array is empty");
	is(ary_avail(&a), (size_t)120, "%zu", "but has a capacity of 120");

	ary_release(&a);

	done_testing();
}
