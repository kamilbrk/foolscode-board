#include <iostream>
#include "skip32.hh"

using namespace std;

int main(int argc, char *argv[])
{
	const std::vector<uint8_t> key = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
//	const std::vector<uint8_t> key = { 'p', 'o', 'i', 'u', 'y', 't', 'r', 'e', 'w', 'q' };
	auto skip = new skip32(key);
	unsigned int plain = 42;
	std::cout << "Encrypted: " << plain << endl;
	unsigned int ecrypt, dcrypt;
	skip->block_encrypt(&plain, &ecrypt);
	std::cout << "Encrypted: " << ecrypt << endl;
	skip->block_decrypt(&ecrypt, &dcrypt);
	std::cout << "Decrypted: " << dcrypt << endl;
	cin.get();
}