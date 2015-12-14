/**
 * @file netcat.cpp
 * @brief netcat(1) implemented using libfoxbox
 */

#include <iostream>
#include <string>
#include "foxbox.h"

using namespace std;
using namespace Foxbox;



int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		Fatal("Usage: %s [-l] address port", argv[0]);
	}
	
	
	bitset<64> key[3];
	key[0] = 0;//0x1eadbeef1eadbeaf;
	key[1] = 0;//0x1eadbeef1eadbeaf;
	key[2] = 0;//0x1eadbeef1eadbeaf;
	for (int i = 3; i < argc; ++i)
	{
		key[i-3] = strtol(argv[i], NULL, 16);
	}
	
	if (strcmp(argv[1], "-l") == 0)
	{
		TCP::Server server(atoi(argv[2]));
		server.Listen();
		DES::Socket des(server,key[0],key[1],key[2]);
		while (des.Valid() && Stdio.Valid())
		{
			Socket::Cat(Stdio, des, des, Stdio);
			//server.Listen(); //uncomment to listen forever
		}
	}
	else
	{
		TCP::Client client(argv[1], atoi(argv[2]));
		DES::Socket des(client,key[0],key[1],key[2]);
		Socket::Cat(Stdio, des, des, Stdio);
	}
}
