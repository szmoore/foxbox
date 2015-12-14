/**
 * @file 3des.cpp
 * @brief Performs Triple DES encryption/decryption on stdin and writes to stdout
 */

#include "foxbox.h"
#include <cassert>

using namespace Foxbox;
using namespace std;

int main(int argc, char ** argv)
{
	
	bitset<64> key[3];
	key[0] = 0x1eadbeef1eadbeaf;
	key[1] = 0x1eadbeef1eadbeaf;
	key[2] = 0x1eadbeef1eadbeaf;

	bool decrypt = false;
	int k = 0;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-d") == 0)
		{
			decrypt = !decrypt;
			continue;
		}
		char * end;
		key[k++] = strtol(argv[i], &end, 16);
		assert(*end == '\0');
	}
	while (Stdio.Valid())
	{
		string input;
		Stdio.GetToken(input, "\n");
		string output1;
		string output2;
		string output3;
		DES::EncryptString(input, output1, key[0],decrypt);
		DES::EncryptString(output1, output2, key[1],!decrypt);
		DES::EncryptString(output2, output3, key[2],decrypt);
		Stdio.Send("%s\n", output3.c_str());
	}
}
