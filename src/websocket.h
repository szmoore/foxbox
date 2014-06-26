#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "tcp.h"
#include "http.h"

#include <sstream>

namespace Foxbox
{
	namespace WS
	{
			/** Used internally by WS::Server and WS::Client **/
			class Socket : public Foxbox::Socket
			{
				public:
					Socket(TCP::Server & server) 
						: Socket(server, false) {}
					Socket(TCP::Client & client) 
						: Socket(client, true) {}
					virtual ~Socket() {}
					virtual bool Send(const char * message, ...);
					virtual bool GetToken(std::string & buffer, 
						const char * delims = " \t\r\n", double timeout=-1, 
						bool inclusive=false);
						
					virtual bool Valid();
					bool GetMessage(std::string & buffer, double timeout=-1);
				protected:
					Socket(TCP::Socket & tcp_socket, bool use_mask);
					

					bool GetMessage(double timeout=-1);
					
					
					TCP::Socket & m_tcp_socket;
					bool m_use_mask;
					bool m_valid;
					std::string m_send_buffer;
					std::string m_recv_buffer;
					std::stringstream m_recv_tokeniser;
				
			};		
			
			class Server : public WS::Socket
			{
				public:
					Server(int port);
					virtual ~Server() {}
					bool Listen();
				private:
					TCP::Server m_server;
			};
			
			class Client : public WS::Socket
			{
				public:
					Client(const char * server_addr, int port, 
						const char * query, const char * proto);
					virtual ~Client() {}
				private:
					TCP::Client m_client;
			};
	}
}

#endif //_WEBSOCKET
