/**
 * @file tcp.cpp
 * @brief TCP Sockets - Definitions
 * @see tcp.h - Declarations
 * @see socket.h - General Socket base class
 * NOTE: Wraps to POSIX networking sockets
 */
 
#include "tcp.h"

using namespace std;

namespace Foxbox {namespace TCP
{
	
unordered_map<int, Server::FDCount> Server::g_portmap;
mutex Server::g_portmap_mutex;

/**
 * Open Socket listening on port 
 */
Socket::Socket(int port) : Foxbox::Socket(), m_port(port)
{
   	Socket::m_sfd = socket(PF_INET, SOCK_STREAM, 0);
   	memset(&m_sockaddr, 0, sizeof(m_sockaddr));
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
	Debug("Closing TCP socket with fd %d", m_sfd);
	if (!Valid()) return;
	if (shutdown(m_sfd, SHUT_RDWR) == -1)
	{
		close(m_sfd); m_sfd = -1;
		// Transport endpoint can become unconnected if the client closes its end; we can't control that.
		Error("Shutting down socket - %s", strerror(errno)); 
	}
	Foxbox::Socket::Close();
}

/** Get address at other end of socket
 */
string Socket::RemoteAddress() const
{
	struct sockaddr remote;
	memset(&remote, 0, sizeof(sockaddr));
	socklen_t len = sizeof(sockaddr);
	if (getpeername(m_sfd, &remote, &len) != 0)
	{
		Error("Error getting peer name - %s", strerror(errno));
		return "disconnected";
	}
	return inet_ntoa(((struct sockaddr_in*)&remote)->sin_addr);
}


/**
 * Construct a Server
 */
Server::Server(int port) : Socket(port)
{
	Server::g_portmap_mutex.lock();
	auto it = Server::g_portmap.find(port);
	if (it != Server::g_portmap.end())
	{
		close(m_sfd); m_sfd=-1;
		m_listen_fd = it->second.fd;
		it->second.count++;
		Server::g_portmap_mutex.unlock();
		return;
	}
	
	
	m_listen_fd = m_sfd;
	m_sfd = -1; 
	// Lots of POSIX boilerplate follows...

	// stop "Address already in use" (we hope... sigh)
	int tmp = 1;
	if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) != 0)
	{
		Fatal("Error in setsockopt(2) - %s", strerror(errno));
	}
	
	struct  sockaddr_in & name = m_sockaddr;
	name.sin_family = AF_INET; // IPv4
	name.sin_addr.s_addr = htonl(INADDR_ANY); // will bind on any interface
	name.sin_port = htons(m_port); // set port

	// bind to address
	if (bind( m_listen_fd, (struct sockaddr *) &name, sizeof(name) ) < 0)
	{
		Fatal("Error binding socket - %s", strerror(errno));
	}
	
	Server::FDCount & fc = TCP::Server::g_portmap[port];
	fc.fd = m_listen_fd;
	fc.count++;
	Server::g_portmap_mutex.unlock();
}

Server::Server(const Server & cpy) : Socket(cpy), m_listen_fd(cpy.m_listen_fd)
{
	TCP::Server::g_portmap_mutex.lock();
	TCP::Server::g_portmap[m_port].count++;
	TCP::Server::g_portmap_mutex.unlock();
}

Server::~Server()
{
	Server::g_portmap_mutex.lock();
	auto it = TCP::Server::g_portmap.find(m_port);
	if (it == TCP::Server::g_portmap.end())
		Fatal("Couldn't find port %d in g_portmap", m_port);
		
	if (it->second.count-- == 1)
	{
		TCP::Server::g_portmap.erase(it);
	}
	Server::g_portmap_mutex.unlock();
}

bool Server::Listen()
{
	if (Socket::Valid())
	{
		Warn("Already have a connection, not listening.");
		return false;
	}
	Close();
	//TODO; optimise listen pool size?
	if (listen(m_listen_fd,1) < 0)
	{
		m_sfd = m_listen_fd;
		Fatal("Error listening - %s", strerror(errno));
		Close();
	}

	m_sfd = accept(m_listen_fd, 0, 0);
	if (m_sfd < 0)
	{
		m_sfd = m_listen_fd;
		Close();
		Error("Error accepting connection - %s", strerror(errno));
	}
	else
	{
		m_file = fdopen(m_sfd, "r+");
		setbuf(m_file, NULL);
		Debug("Got connection");
	}
	return true;
}

/**
 * Construct a Client (ie: Connect to address:port)
 */
Client::Client(const char * server_address, int port) : Socket(port)
{
	struct sockaddr_in & server = m_sockaddr;
	struct  hostent *hp;


	server.sin_family = AF_INET; //IPv4
	hp = gethostbyname(server_address); // get host
	bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length); // no idea what this does
	m_sockaddr.sin_port = htons(m_port); // set the port

	// try connecting
	if (connect(m_sfd, (struct sockaddr *) &server, sizeof(sockaddr_in)) < 0)
	{
		Error("Error connecting to server at address %s:%d - %s ", server_address, port, strerror(errno));
		Close();
		Fatal("Couldn't create TCP::Client");
	}
}



}} // end namespaces
