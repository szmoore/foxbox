/**
 * @file wsserver.cpp
 * @brief A Websocket server example
 */
 
#include "foxbox.h"
#include <unistd.h>

using namespace std;
using namespace Foxbox;
 
int main(int argc, char ** argv)
{
	int port = (argc == 2) ? atoi(argv[1]) : 7681;
	
	Socket input(stdin);
	Socket output(stdout);
	while (true)
	{
		WS::Server server(port);
		while (true)
		{
			Debug("Listen");
			server.Listen();
			Debug("Connected");
			if (!server.Valid())
			{
				Debug("Invalide!");
				HTTP::SendPlain(server, 400, "This is a WebSocket server");
				server.Close();
				continue;
			}
			while (server.Valid())
			{
				server.Send("Hello, world!\n");
				sleep(1);
			}
			server.Close();
		}
	}
}
