// skip32.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include "skip32.hh"

int main(int argc, char *argv[])
{
	std::vector<uint8_t> key;
	key.assign(argv[1], argv[1] + 10);
	auto skip = new skip32(key);
	int plain, encrypt; 
	unsigned int out;
	sscanf_s(argv[2], "%d", &plain);
	sscanf_s(argv[3], "%d", &encrypt);
	if (encrypt)
		skip->block_encrypt(&plain, &out);
	else 
		skip->block_decrypt(&plain, &out);
	std::cout << out;
	delete skip;
    return 0;
}

