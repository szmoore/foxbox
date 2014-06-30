/**
 * @file websocket.h
 * @brief WebSocket protocol using TCP::Socket's - Declarations
 * @see websocket.cpp - Definitions
 * @see tcp.h - TCP::Sockets
 * @see RFC-6455 http://tools.ietf.org/html/rfc6455 
 * 	NOTE: This is NOT fully RFC complaint
 */

#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "tcp.h"
#include "http.h"

#include <sstream>

namespace Foxbox
{
	namespace WS
	{
			/**
			 * A WebSocket based on Foxbox::Socket 
			 *  You should not construct this class; use WS::Server and
			 * 	WS::Client instead
			 * NOTE: THIS DOES NOT INHERIT FROM TCP::Socket
			 * 	The relationship is a "uses" rather than "is"
			 */
			class Socket : public Foxbox::Socket
			{
				//(Inheriting from TCP::Socket is a trap that leads to 
				//	C++ inheritance nightmares)
				public:
					virtual ~Socket() {}
					
					// override virtual functions of Foxbox::Socket
					virtual bool Send(const char * message, ...);
					virtual bool GetToken(std::string & buffer, 
						const char * delims = " \t\r\n", double timeout=-1, 
						bool inclusive=false);
					virtual bool Get(std::string & buffer, unsigned num_chars, double timeout = -1);
					virtual bool Valid();
					
					TCP::Socket & TCP() {return m_tcp_socket;}

					/** Get message in its entirity **/
					bool GetMessage(std::string & buffer, double timeout=-1);
				protected:
					/** Constructor used by WS::Server **/
					Socket(TCP::Server & server)
						: Socket(server, false) {}
					/** Constructor used by WS::Client **/
					Socket(TCP::Client & client) 
						: Socket(client, true) {}
					
					/** Sending/Receing uses this TCP socket **/
					TCP::Socket & m_tcp_socket; 
					bool m_use_mask;
					bool m_valid;
					std::string m_send_buffer;
					std::string m_recv_buffer;
					std::stringstream m_recv_tokeniser;
					
				private:
					/** Other constructors wrap to this **/
					Socket(TCP::Socket & tcp_socket, bool use_mask);
					bool GetMessage(double timeout=-1);
			};		
			
			/** A WebSocket Server **/
			class Server : public WS::Socket
			{
				public:
					Server(int port);
					virtual ~Server() {}
					bool Listen();
				private:
					TCP::Server m_server;
			};
			
			/** A WebSocket Client **/
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
