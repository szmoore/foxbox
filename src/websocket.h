#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "tcp.h"
#include "http.h"

namespace Foxbox
{
	namespace WS
	{
			class Server
			{
				public:
					Server(TCP::Socket & socket);
					virtual ~Server() {}
					
					bool Valid();
				private:
					TCP::Socket & m_socket;
					HTTP::Request m_handshake;
					bool m_valid;
					std::string m_magic;
			};

	}
}

#endif //_WEBSOCKET
