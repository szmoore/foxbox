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
		//Socket::Dump(client, snull);
		
		// check the output of the process and the output of the CGI are the same
		Process proc("cgi.test");
		string same; string diff1; string diff2;
		int not_identical = Socket::Compare(proc, client, &same, &diff1, &diff2);
		if (not_identical > 0)
		{
			Error("Expected output does not match with actual output");
			Error("First difference at position %d", not_identical);
			Error("Same: %s", same.c_str());
			Error("Process: ... %s", diff1.c_str());
			Error("CGI response: ... %s", diff2.c_str());
			Fatal("Stopping stress test."); 
		}
		
	}
}
#define POOL_SIZE 1

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
