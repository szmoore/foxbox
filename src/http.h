#ifndef _HTTP_H
#define _HTTP_H

#include <map>
#include <string>

#include "socket.h"

namespace Foxbox
{
	namespace HTTP
	{		
			class Request
			{
				public:
					Request(const std::string & hostname = "",
							const std::string & request_type = "",
							const std::string & query = "");
					virtual ~Request();
					
					std::map<std::string, std::string> & Params() {return m_params;}
					std::map<std::string, std::string> & Cookies() {return m_cookies;}
					std::map<std::string, std::string> & Headers() {return m_headers;}
					std::string & Path() {return m_path;}
					
					bool Receive(Socket & socket);
					bool Send(Socket & socket);
					
					std::vector<std::string> & SplitPath(char delim = '/');
					
				private:
					std::string m_hostname;
					std::string m_request_type;
					std::string m_query;
					std::string m_path;
					std::map<std::string, std::string> m_params;
					std::map<std::string, std::string> m_cookies;
					std::map<std::string, std::string> m_headers;
					std::vector<std::string> * m_split_path;
			};
			extern void SendJSON(Socket & socket, const std::map<std::string, std::string> & m, bool with_header = true);
			extern void SendFile(Socket & socket, const std::string & filename, bool with_header = true);
			extern void FormQuery(std::string & s, const std::map<std::string, std::string> & m, char seperator='&', char equals = '=');
			extern std::string ParseQuery(std::map<std::string, std::string> & m, const std::string & s, char start = '?', char seperator='&', char equals = '=',const char * strip = " \r\n;:");
			
			inline void strip(std::string & s, const char * delims)
			{
				while (s.size() > 0 && strchr(delims, s.front())) 
					s.erase(s.begin());
				while (s.size() > 0 && strchr(delims, s.back()))
					s.pop_back();
			}
	}
}

#endif //_HTTP_H



