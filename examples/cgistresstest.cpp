/**
 * @file httpserver.cpp
 * @brief Simple single threaded HTTP API implemented using libfoxbox
 *
 */
 
#include "foxbox.h"
#include <map>
#include <sstream>
#define Debug(...) Log(LOGDEBUG, __VA_ARGS__)
using namespace std;
using namespace Foxbox;

bool g_running = true;

void Client(int port, int id)
{
	int count = 0;

	Socket snull(fopen("/dev/null", "w"));
	while (g_running)
	{
		TCP::Client client("localhost", port);
		HTTP::Request req("localhost", "GET", "/cgi/cgi.test");
		req.Send(client);
		Debug("Sent request %d", ++count);
		int status = HTTP::ParseResponseHeaders(client, NULL, NULL);
		Debug("Got response, status %d", status);
		Socket::Dump(client, snull);
	}
}
#define POOL_SIZE 10

int main(int argc, char ** argv)
{
	int port = (argc == 2) ? atoi(argv[1]) : 8080;
	thread pool[POOL_SIZE];
	for (unsigned i = 0; i < POOL_SIZE; ++i)
	{
		Debug("Client thread %d", i);
		pool[i] = thread(Client,port, i);
	}
	
	for (unsigned i = 0; i < POOL_SIZE; ++i)
	{
		pool[i].join();
		Debug("Joined thread %d", i);
	}
	Debug("All threads done.");

}
