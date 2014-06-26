/**
 * @file wsserver.cpp
 * @brief A Websocket server example
 */
 
#include "foxbox.h"

using namespace std;
using namespace Foxbox;
 
int main(int argc, char ** argv)
{
	int port = (argc == 2) ? atoi(argv[1]) : 7681;
	while (true)
	{
		TCP::Server server(port);
		server.Listen();
		while (server.Valid())
		{
			WS::Server ws(server);
			Debug("Tried to websocket it... valid is %d", ws.Valid());
			if (!ws.Valid())
				HTTP::SendPlain(server, 400, "This is a Websocket server.");	
			else do 
			{
				//Debug("Connected!");

			} while (ws.Valid());
			
			server.Close();
			server.Listen();
		}
		
	}
}
