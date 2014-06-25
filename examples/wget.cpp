/**
 * @file wget.cpp
 * @brief wget using libfoxbox
 * Note most of the work is argument parsing.
 */

#include "foxbox.h"

using namespace std;
using namespace Foxbox;

int main(int argc, char ** argv)
{
	if (argc < 2)
		Fatal("Usage: %s url [output]", argv[0]);
	
	FILE * o = (argc > 2) ? fopen(argv[2], "w") : stdout;
	Socket output(o);
	int port = 80;
	char * url = argv[1];
	char * path = strchr(url, '/');
	char * p = strchr(url, ':');
	
	if (path != NULL && *path != '\0')
	{
		*path = '\0';
		++path;
		
	}
	else
	{
		path = (char*)("");
	}
	
	if (p != NULL && *p != '\0') 
	{
		*p = '\0';
		port = atoi(p+1);
	}
	TCP::Client input(url, port);
	HTTP::Request req(url, "GET", path);
	req.Send(input);
	Socket::Cat(input, output, input, output);
}
