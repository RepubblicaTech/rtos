#include <assert.h>

#include <stdio.h>

extern void _hcf();

void assert_impl(const char *function, int line, bool condition, char *condtion_str) {
	if (!condition) {
		kprintf("--- [ PANIC @ %s():%d ] --- Condition <%s> failed!\n", function, line, condtion_str);

		_hcf();
	}
}
