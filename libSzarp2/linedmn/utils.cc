#include "utils.h"

#include <iostream>

void print_as_hex(const std::vector<unsigned char> &buf)
{
	int size = buf.size();
	for (int i = 0; i < (size - 1); i++) {
		std::cout << std::hex << (int)buf[i] << ':';
	}
	std::cout << std::hex << (int)buf[size - 1] << std::endl;
}
