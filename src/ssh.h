#ifndef _SSH_H
#define _SSH_H

#include "tcp.h"

#include <bitset>

namespace Foxbox
{
	namespace SSH
	{
		class Socket : public TCP::Socket
		{
				public:
					virtual ~Socket() {}
					
					// override virtual functions of Foxbox::Socket
					virtual bool Send(const char * message, ...);
					virtual bool GetToken(std::string & buffer, 
						const char * delims = " \t\r\n", double timeout=-1, 
						bool inclusive=false);
					virtual bool Get(std::string & buffer, unsigned num_chars, double timeout = -1);
					virtual bool Valid();
					inline bool Send(const std::string & buffer) {return Send(buffer.c_str());}
					virtual void Close() {m_tcp_socket.Close();}
					
					TCP::Socket & TCP() {return m_tcp_socket;}

					/** Get message in its entirity **/
					bool GetMessage(std::string & buffer, double timeout=-1);
					
					
				protected:
					Socket(TCP::Socket & tcp_socket);
					
					/** Sending/Receing uses this TCP socket **/
					TCP::Socket & m_tcp_socket; 
					bool m_use_mask;
					bool m_valid;
					std::string m_send_buffer;
					std::string m_recv_buffer;
					std::stringstream m_recv_tokeniser;
					
				private:
					bool GetMessage(double timeout=-1);			
		};
		
		class Client : public SSH::Socket
		{
			public:
				Client(const char * addr, int port);
				virtual ~Client();
			private:
				TCP::Client m_client;
		};
	}


#endif //_SSH_H
