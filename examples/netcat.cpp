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
	
	//Socket input(stdin);
	//Socket output(stdout);
	
	if (strcmp(argv[1], "-l") == 0)
	{
		TCP::Server server(atoi(argv[2]));
		server.Listen();
		while (server.Valid() && Stdio.Valid())
		{
			Socket::Cat(Stdio, server, server, Stdio);
			//server.Listen(); //uncomment to listen forever
		}
	}
	else
	{
		TCP::Client client(argv[1], atoi(argv[2]));
		Socket::Cat(Stdio, client, client, Stdio);
	}
}
