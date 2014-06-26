#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "tcp.h"
#include "http.h"

namespace Foxbox
{
	namespace WS
	{
			class Server : public TCP::Server
			{
				public:
					Server(int port);
					virtual ~Server() {}
					void Listen();
					bool Valid();
					bool Send(const char * message, ...);
				private:
					bool m_valid;
			};
			
			

	}
}

#endif //_WEBSOCKET
