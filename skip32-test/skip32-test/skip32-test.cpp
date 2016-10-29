#include <iostream>
#include "skip32.hh"

using namespace std;

int main(int argc, char *argv[])
{
	const std::vector<uint8_t> key = { 0x9b, 0x21, 0x96, 0xe, 0x1a, 0xcf, 0x24, 0x5f, 0x14, 0x93 };
//	const std::vector<uint8_t> key = { 'p', 'o', 'i', 'u', 'y', 't', 'r', 'e', 'w', 'q' };
	auto skip = new skip32(key);
	int plain = 42;
	std::cout << "Encrypted: " << plain << endl;
	int ecrypt, dcrypt;
	skip->block_encrypt(&plain, &ecrypt);
	std::cout << "Encrypted: " << ecrypt << endl;
	skip->block_decrypt(&ecrypt, &dcrypt);
	std::cout << "Decrypted: " << dcrypt << endl; 
	cin.get();
}