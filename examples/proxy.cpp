/**
 * @file proxy.cpp
 * @brief TCP Proxy Server
 * Listens for TCP connections then cats between them and a remote server
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
		TCP::Client client(target, target_port);
		Socket::Cat(client, server, server, client);
		client.Close();
		server.Close();
	}
}
