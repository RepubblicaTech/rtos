/*
	Check if a condition is true, panics if not

	Source: https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/libk/testing/assert.c
*/

#ifndef ASSERT_H
#define ASSERT_H 1

#include <stdbool.h>

#define assert(condition)	assert_impl(__FUNCTION__, __LINE__, condition, #condition)

void assert_impl(const char *function, int line, bool condition, char *condtion_str);

#endif