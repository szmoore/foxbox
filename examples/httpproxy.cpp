/**
 * @file httpproxy.cpp
 * @brief HTTP Proxy Server
 * Same as the TCP proxy example, but checks the HTTP requests are valid
 * @see proxy.cpp
 */

#include "foxbox.h"

using namespace std;
using namespace Foxbox;

int main(int argc, char ** argv)
{
	
	if (argc < 2)
		Fatal("Usage: %s target_address [target_port=80] [listen_port=8080]", argv[0]);
	
	char * target = argv[1];
	int target_port = (argc > 2) ? atoi(argv[2]) : 80;
	int listen_port = (argc > 3) ? atoi(argv[3]) : 8080;
	
	TCP::Server server(listen_port);
	while (true)
	{
		server.Listen();
		HTTP::Request req;
		if (req.Receive(server))
		{
			Debug("Forwarding request...");
			TCP::Client client(target, target_port);
			Socket::Cat(client, server, server, client);
			client.Close();
		}
		else
		{
			Debug("Bad Request.");
			server.Send("Bad Request.");
		}
		server.Close();
	}
}
