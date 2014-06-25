/**
 * @file network.cpp
 * @brief Wrappers for low level POSIX networking - Implementations
 */
 
#include "tcp.h"

using namespace std;

namespace Foxbox {namespace TCP
{

/**
 * Open Socket listening on port 
 */
Socket::Socket(int port) : Foxbox::Socket(), m_port(port)
{
   	Socket::m_sfd = socket(PF_INET, SOCK_STREAM, 0);
   	
	if (m_sfd < 0)
	{
		Fatal("Error creating TCP socket");
	}
}

/**
 * Close TCP socket
 */
void Socket::Close()
{
	//Debug("Closing TCP socket");
	if (!Valid()) return;
	if (shutdown(m_sfd, SHUT_RDWR) == -1)
	{
		close(m_sfd); m_sfd = -1;
		Fatal("Shutting down socket - %s", strerror(errno));
	}
	Foxbox::Socket::Close();
}


/**
 * Construct a Server (ie: Listen for connections on port)
 */
Server::Server(int port) : Socket(port)
{
	m_listen_fd = m_sfd;
	m_sfd = -1; 
	// Lots of POSIX boilerplate follows...

	// stop "Address already in use" (we hope... sigh)
	int tmp = 1;
	if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) != 0)
	{
		Fatal("Error in setsockopt(2) - %s", strerror(errno));
	}
	
	struct   sockaddr_in name;
	name.sin_family = AF_INET; // IPv4
	name.sin_addr.s_addr = htonl(INADDR_ANY); // will bind on any interface
	name.sin_port = htons(m_port); // set port

	// bind to address
	if (bind( m_listen_fd, (struct sockaddr *) &name, sizeof(name) ) < 0)
	{
		Fatal("Error binding socket - %s", strerror(errno));
	}
}

bool Server::Listen()
{
	if (Socket::Valid())
	{
		Warn("Already have a connection, not listening.");
		return false;
	}
	//TODO; optimise listen pool size?
	if (listen(m_listen_fd,1) < 0)
	{
		m_sfd = m_listen_fd;
		Close();
		Fatal("Error listening - %s", strerror(errno));
	}

	m_sfd = accept(m_listen_fd, 0, 0);
	if (m_sfd < 0)
	{
		m_sfd = m_listen_fd;
		Close();
		Fatal("Error accepting connection - %s", strerror(errno));
	}
	m_file = fdopen(m_sfd, "r+");
	setbuf(m_file, NULL);
	//Debug("Got connection");
	return true;
}

/**
 * Construct a Client (ie: Connect to address:port)
 */
Client::Client(const char * server_address, int port) : Socket(port)
{
	struct	sockaddr_in server;
	struct  hostent *hp;


	server.sin_family = AF_INET; //IPv4
	hp = gethostbyname(server_address); // get host
	bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length); // no idea what this does
	server.sin_port = htons(m_port); // set the port

	// try connecting
	if (connect(m_sfd, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		Error("Error connecting to server at address %s:%d - %s ", server_address, port, strerror(errno));
		Close();
		Fatal("Couldn't create TCP::Client");
	}
}



}} // end namespaces
