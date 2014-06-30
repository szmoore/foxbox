/**
 * @file tcp.h
 * @brief TCP Sockets - Declarations
 * @see tcp.cpp - Definitions
 * @see socket.h - General Socket base class
 */
#ifndef _TCP_H
#define _TCP_H


#include <unordered_map>
#include <list>
#include <mutex>

/** Custom includes **/
#include "socket.h"


namespace Foxbox
{
	/** Classes for TCP/IPv4 networking **/
	namespace TCP
	{
		/**
		 * A TCP Socket based on Foxbox::Socket 
		 * You should not construct this class directly, use TCP::Server
		 * 	or TCP::Client
		 */
		class Socket : public Foxbox::Socket
		{
			public:
				virtual ~Socket() {Close();}
				virtual void Close();
			protected:
				/** Should not construct this class directly **/
				Socket(int port);
				Socket(const Socket & cpy) : Foxbox::Socket(cpy), m_port(cpy.m_port) {}
				int m_port; /** Port being used **/
		};
		
		/** A TCP Socket opened as a Server (ie: Listens for connections) **/
		class Server : public Socket
		{
			public:
				Server(int port); /** Open and listen for connections **/
				Server(const Server & cpy);
				virtual ~Server();
				bool Listen();
				

			private:
				int m_listen_fd;
				/** bound FD and number of Server's using it **/
				typedef struct FDCount
				{
					FDCount() : fd(0), count(0) {}
					int fd;
					int count;
				} FDCount;
				/** Map ports to FDCount **/
				static std::unordered_map<int, FDCount> g_portmap;
				/** Mutex **/
				static std::mutex g_portmap_mutex;
		};
		
		/** A TCP Socket opened as a Client (ie: Connects to address:port)**/
		class Client : public Socket
		{
			public:
				Client(const char * server_addr, int port); /** Connect to IPv4 address **/
				Client(const Client & cpy) : Socket(cpy) {}
				virtual ~Client() {}
		};
	}
}

#endif //_TCP_H
