/**
 * @file httpserver.cpp
 * @brief Simple single threaded HTTP API implemented using libfoxbox
 *
 */
 
#include "foxbox.h"
#include <map>
#include <sstream>

using namespace std;
using namespace Foxbox;
int main(int argc, char ** argv)
{
	int port = (argc == 2) ? atoi(argv[1]) : 8080;
	int count = 0;
	while (true)
	{
		TCP::Server server(port);
		server.Listen();
		while (server.Valid())
		{
			Debug("Connected; wait for request");
			HTTP::Request req;
			if (!req.Receive(server))
			{
				Debug("Invalid request!");
				server.Close();
				server.Listen();
				continue;
			}
			Debug("Got request! Path is: %s", req.Path().c_str());
		
			string & api = req.SplitPath().front();
			if (api == "cookies")
			{
				HTTP::SendJSON(server, req.Cookies());
			}
			else if (api == "headers")
			{
				HTTP::SendJSON(server, req.Headers());
			}
			else if (api == "echo")
			{
				HTTP::SendJSON(server, req.Params());
			}
			else if (api == "meta")
			{
				stringstream s; s << ++count;
				map<string,string> m;
				m["request_number"] = s.str();
				HTTP::SendJSON(server, m);
			}
			else if (api == "file")
			{
				HTTP::SendFile(server, req.SplitPath().back());
			}
			else
			{
				HTTP::SendFile(server, "index.html");
			}
			server.Close();
			server.Listen();
		}
	}
}
