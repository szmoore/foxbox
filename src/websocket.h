#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "tcp.h"
#include "http.h"

namespace Foxbox
{
	namespace WS
	{
			class Protocol
			{
				private:
					friend class Server;
					friend class Client;
					
					Protocol() : m_send_buffer(""), m_recv_buffer("") {}
					virtual ~Protocol() {}
					
					bool Send(Socket & socket, int mask, const char * message, va_list ap);
					bool GetToken(std::string & buffer, 
						const char * delims = " \t\r\n", double timeout=-1, 
						bool inclusive=false);
						
					bool GetMessage(Socket & socket, double timeout=-1);
					
					std::string m_send_buffer;
					std::string m_recv_buffer;
				
			};
			
			
			class Server : public TCP::Server, WS::Protocol
			{
				public:
					Server(int port) : TCP::Server(port), Protocol(), 
										m_valid(false) {}
					virtual ~Server() {}
					void Listen();
					bool Valid();
					virtual bool Send(const char * message, ...);
					bool GetMessage(std::string & buffer, double timeout=-1);
					
					
				private:
					bool m_valid;
			};
			
			class Client : public TCP::Client, WS::Protocol
			{
				public:
					Client(const char * address, int port);
					virtual ~Client() {}
					bool Valid();
					virtual bool Send(const char * message, ...);
					bool GetMessage(std::string & buffer, double timeout=-1);
			};
			
			

	}
}

#endif //_WEBSOCKET
