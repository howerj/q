#include <iostream>
#include "q.h"

/* This is just a test program to make sure that the C header does not use C
 * only features or do anything weird. A proper C++ wrapper, that would allow
 * one to write idiomatic C++ code would be helpful, although it might require
 * changes to its interface. */

int main(void) {
	q_t a = qint(1), b = qint(1);
	q_t c = qadd(a, b);
	std::cout << c << std::endl;
	return 0;
}

