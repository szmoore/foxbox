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
				HTTP::SendPlain(server.TCP(), 400, "This is a WebSocket server");
				server.Close();
				continue;
			}
			while (server.Valid())
			{
				Debug("Send Ping! %d", server.Send("Ping!\n"));
				
				sleep(1);
				string message("");
				//server.GetMessage(message);
				// GetToken returns false if the token is empty.
				if (server.GetToken(message, "\n"))
					Debug("Response is %s", message.c_str());
				//else
				//	Debug("Response is empty.");
				
			}
			server.Close();
		}
	}
}
