/**
 * @file http.h
 * @brief HTTP helper classes and functions - Declarations
 * @see http.cpp - Definitions
 * @see socket.h - General Socket base class
 * @see RFC 2616 http://www.w3.org/Protocols/rfc2616/rfc2616.html
 * 	NOTE: This is NOT fully RFC complaint
 */
 
#ifndef _HTTP_H
#define _HTTP_H

// All sets of key,value pairs are represented by map<string,string>
#include <map> 
#include <string>

#include "tcp.h"

namespace Foxbox
{
	namespace HTTP
	{		
			/**
			 * Helper class used for _both_ forming and receiving HTTP requests
			 * Note: This does not inherit from Foxbox::Socket
			 * 	Usage is to create Socket(s) seperately and pass to the 
			 *   member functions of this class
			 * @see examples/httpserver.cpp
			 * @see examples/httpproxy.cpp
			 * @see examples/wget.cpp
			 */
			class Request
			{
				public:
					Request(const std::string & hostname = "",
							const std::string & request_type = "",
							const std::string & query = "");
					virtual ~Request();
					
					/** Access query parameters @returns mutable reference **/
					std::map<std::string, std::string> & Params() {return m_params;}
					/** Access cookie parameters @returns mutable reference **/
					std::map<std::string, std::string> & Cookies() {return m_cookies;}
					/** Access headers @returns mutable reference **/
					std::map<std::string, std::string> & Headers() {return m_headers;}
					/** Access path string @returns mutable reference **/
					std::string & Path() {return m_path;}
					
					/** Receive a HTTP request over a Foxbox::Socket **/
					bool Receive(Socket & socket);
					/** Send a HTTP request over a Foxbox::Socket **/
					bool Send(Socket & socket);
					
					bool Valid() const {return m_valid;}
					
					/** Split the path part of the URL **/
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
					bool m_valid;
			};
			
			/** Form a query string fom a map<string,string> of {key,value} pairs **/
			extern void FormQuery(std::string & s, const std::map<std::string, std::string> & m, char seperator='&', char equals = '=');
			/** Parse a query/cookie string to form a map<string,string> of {key,value} pairs **/
			extern std::string ParseQuery(std::map<std::string, std::string> & m, const std::string & s, char start = '?', char seperator='&', char equals = '=',const char * strip = " \r\n;:");
			
			/** Send JSON over a socket **/
			extern bool SendJSON(Socket & socket, const std::map<std::string, std::string> & m, unsigned status=0);
			/** Send file **/
			extern bool SendFile(Socket & socket, const char * filename, unsigned status=200);
			inline bool SendFile(Socket & socket, const std::string & filename, unsigned status=200)
			{
				return SendFile(socket, filename.c_str(), status);
			}
			/** Send plain text **/
			extern bool SendPlain(Socket & socket, unsigned status, const char * message="");
			inline bool SendPlain(Socket & socket, const char * message="") {return SendPlain(socket, 200, message);}
			
			/** Expects a HTTP response, returns the status code **/
			extern unsigned ParseResponseHeaders(Socket & socket,
				std::map<std::string, std::string> * m=NULL, std::string * reason=NULL);

			/** Helper to split unwanted characters from HTTP headers and lines **/
			inline void strip(std::string & s, const char * delims = "\t \r\n:")
			{
				while (s.size() > 0 && strchr(delims, s.front())) 
					s.erase(s.begin());
				while (s.size() > 0 && strchr(delims, s.back()))
					s.pop_back();
			}
			
			extern const char * StatusMessage(unsigned code);
			/** 
			 * Update one map with the contents of another
			 * Different from std::map::merge ; existing values are overwritten
			 * @param dest Map to update
			 * @param src Map to update with
			 * @returns dest
			 */
			inline std::map<std::string, std::string> & Update(
				std::map<std::string, std::string> & dest, 
				const std::map<std::string, std::string> & src)
			{
				for (auto & it : src)
					dest[it.first] = it.second;
				return dest;
			}
	}
}

#endif //_HTTP_H



