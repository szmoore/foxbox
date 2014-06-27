/**
 * @file threadedclient.cpp
 * @brief Multithreaded TCP client
 */
 
#include <thread>
#include "foxbox.h"

using namespace std;
using namespace Foxbox;
 
#define POOL_SIZE 4

void Run(const char * address, int port, int id)
{
	TCP::Client client(address, port);
	Debug("Thread %d constructed client to %s:%d", id,address,port);
	while (client.Valid())
	{
		//Debug("Sending...");
		if (!client.Send("Hello from client thread %d\n", id))
		{
			Warn("Failed to send message\n");
		}
		//Debug("Sent!");
		string s("");
		client.GetToken(s, "\n");
		Debug("Response in %d: %s", id, s.c_str());
	}
	client.Close();
	Debug("Thread %d exits.", id);
}

int main(int argc, char ** argv)
{
	const char * address = (argc > 1) ? argv[1] : "localhost";
	int port = (argc > 2) ? atoi(argv[2]) : 6666;
	
	thread pool[POOL_SIZE];
	for (unsigned i = 0; i < POOL_SIZE; ++i)
	{
		Debug("Client thread %d", i);
		pool[i] = thread(Run,address, port, i);
	}
	
	for (unsigned i = 0; i < POOL_SIZE; ++i)
	{
		pool[i].join();
		Debug("Joined thread %d", i);
	}
	Debug("All threads done.");
}
