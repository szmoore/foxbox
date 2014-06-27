/**
 * @file threadedserver.cpp
 * @brief Excample TCP server using fixed number of threads to maintain multiple connections
 */

#include "foxbox.h"

#include <thread>

using namespace std;
using namespace Foxbox;

#define POOL_SIZE 4

void Serve(int port, int id)
{
	TCP::Server server(port);
	Debug("Thread %d constructed server on port %d", id,port);
	server.Listen();
	Debug("Thread %d got a client", id);
	while (server.Valid())
	{
		string line;
		server.GetToken(line,"\n", -1, true);
		server.Send(line);
	}
	server.Close();
	Debug("Thread %d exits.", id);
}

int main(int argc, char ** argv)
{
	int port = (argc > 1) ? atoi(argv[1]) : 6666;
	
	
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
