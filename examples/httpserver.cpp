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

bool g_running = true;

void Serve(int port, int id)
{
	int count = 0;
	TCP::Server server(port);
	while (g_running)
	{
		
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
				s.clear();
				s << id;
				m["thread_number"] = s.str();
				HTTP::SendJSON(server, m);
			}
			else if (api == "file")
			{
				HTTP::SendFile(server, req.SplitPath().back());
			}
			else if (api == "cgi")
			{
				Debug("Got CGI request");
				req.CGI(server, req.SplitPath().back().c_str());
				Debug("Finished parsing CGI request.");
			}
			else if (api == "quit")
			{
				g_running = false;
				HTTP::SendPlain(server, 200, "Dying now.");
				Fatal("Quit.");
			}
			else
			{
				HTTP::SendFile(server, "index.html");
			}
			server.Close();
			if (g_running)
				server.Listen();
		}
	}
}
#define POOL_SIZE 10

int main(int argc, char ** argv)
{
	int port = (argc == 2) ? atoi(argv[1]) : 8080;
	thread pool[POOL_SIZE];
	for (unsigned i = 0; i < POOL_SIZE; ++i)
	{
		Debug("Serve thread %d", i);
		pool[i] = thread(Serve,port, i);
	}
	
	for (unsigned i = 0; i < POOL_SIZE; ++i)
	{
		pool[i].join();
		Debug("Joined thread %d", i);
	}
	Debug("All threads done.");

}
