#include <iostream>
#include "q.hpp"

/* Test program for Q library C++ Wrapper */

int main(void) {
	Q a(1), b = 2, c;
	a += 3;
	++b;
	c = a+b;
	std::cout << (int)c << std::endl;
	std::cout << (int)(b * b) << std::endl;
	return 0;
}

