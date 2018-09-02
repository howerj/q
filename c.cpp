#include <iostream>
#include "q.hpp"

/* Test program for Q library C++ Wrapper */

int main(void) {
	Q a(1), b = 2, c, d;
	a += 3;
	++b;
	c = a + b;
	d = b * b;
	std::cout << (int)c << std::endl;
	std::cout << (int)d << std::endl;
	std::cout << (int)(d.sqrt()) << std::endl;
	return 0;
}

