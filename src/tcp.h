/**
 * @file network.h
 * @brief Wrappers for low level POSIX networking - Declarations
 */
#ifndef NETWORK_H
#define NETWORK_H

/** Custom includes **/
#include "socket.h"


namespace Foxbox
{
	/** Classes for TCP/IPv4 networking **/
	namespace TCP
	{
		/** A TCP Socket **/
		class Socket : public Foxbox::Socket
		{
			public:
				virtual ~Socket() {Close();}
				virtual void Close();
			protected:
				/** NB: Should not construct this class directly, use Server or Client **/
				Socket(int port);
				int m_port; /** Port being used **/
		};
		
		/** A TCP Socket opened as a Server (ie: Listens for connections) **/
		class Server : public Socket
		{
			public:
				Server(int port = 4560); /** Open and listen for connections **/
				virtual ~Server() {}
				bool Listen();
			private:
				int m_listen_fd;
		};
		
		/** A TCP Socket opened as a Client (ie: Connects to address:port)**/
		class Client : public Socket
		{
			public:
				Client(const char * server_addr = "127.0.0.1", int port = 4560); /** Connect to IPv4 address **/
				virtual ~Client() {}
		};
	}
}

#endif //NETWORK_H
