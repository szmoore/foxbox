/**
 * @file wscat.cpp
 * @brief Implement netcat(1) but using libfoxbox WebSockets (Foxbox::WS)
 */

#include "foxbox.h"
#include <unistd.h>


using namespace std;
using namespace Foxbox;

int main(int argc, char ** argv)
{
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	if (argc < 3)
	{
		Fatal("Usage: Server: %s -l port\n\t Client: %s address port [query=/] [proto=wscat]", argv[0]);
	}
	
	Socket input(stdin);
	Socket output(stdout);
	
	if (strcmp(argv[1], "-l") == 0)
	{
		WS::Server server(atoi(argv[2]));
		server.Listen();
		if (!server.Valid())
		{
			Error("Handshake invalid.");
		}
		while (server.Valid() && input.Valid() && output.Valid())
		{
			Socket::Cat(input, server, server, output);
			//server.Listen(); //uncomment to listen forever
		}
	}
	else
	{
		const char * query = (argc > 3) ? argv[3] : "/";
		const char * proto = (argc > 4) ? argv[4] : "wscat";
		WS::Client client(argv[1], atoi(argv[2]), query, proto);
		//Debug("Done handshake, valid is %d, %d", client.Valid(), client.Foxbox::Socket::Valid());
		//Debug("Send %d", client.Send("Hello!"));
		
		Socket::Cat(client, output, input, client);
		//Debug("Done Socket::Cat, valid is %d, %d", client.Valid(), client.Foxbox::Socket::Valid());
	}
}
