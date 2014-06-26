/**
 * @file socket.h
 * @brief Low level POSIX socket wrappers - Declarations
 */
 
 #ifndef _SOCKET_H
 #define _SOCKET_H
 
/** C includes **/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

/** C++ includes **/
#include <string>
#include <vector>

/** Custom includes **/
#include "log.h"
 
namespace Foxbox
{
	class Socket
	{
		public:
			Socket() : m_sfd(-1), m_file(NULL) {}
			Socket(FILE * file) : m_sfd(-1), m_file(file) {if (m_file != NULL) m_sfd = fileno(m_file);}
			virtual ~Socket() {Close();}
			virtual void Close();
			virtual bool Valid(); /** Socket can be read/written from/to **/
			virtual bool Send(const char * print, ...);  /** Send formatted message **/
			bool Send(const std::string & buffer) {return Send(buffer.c_str());} /** Send C++ string **/
			inline bool GetToken(std::string & buffer, char delim, double timeout=-1) {return GetToken(buffer, ""+delim, timeout);}
			virtual bool GetToken(std::string & buffer, const char * delims = " \t\r\n", double timeout=-1, bool inclusive=false); /** Read until delimeter or timeout **/
			bool Get(std::string & buffer, unsigned num_chars, double timeout = -1); /** Read number of characters or timeout **/
			/** Select first available for reading from **/
			static Socket * Select(const std::vector<Socket*> & sockets);
			static Socket * Select(unsigned size, Socket * sockets);
			static Socket * Select(Socket * s1, ...);
			/** Implements cat **/
			static void Cat(Socket & in1, Socket & out1, Socket & in2, Socket & out2);
			bool CanReceive(double timeout=-1);
			
			size_t Write(void * data, size_t size)
			{
				size_t result = fwrite(data, 1, size, m_file);
				/*
				if (feof(m_file))
				{
					Debug("Got EOF!");
				}
				if (ferror(m_file))
				{
					Debug("Got Error!");
				}
				*/
				return result;
			}
			size_t Read(void * data, size_t size)
			{
				size_t result = fread(data, 1, size, m_file);
				/*
				if (feof(m_file))
				{
					Debug("Got EOF!");
				}
				if (ferror(m_file))
				{
					Debug("Got Error!");
				}
				*/
				return result;
			}
			
			int GetFD() const {return m_sfd;} // use with caution...
			
		protected:	
			int m_sfd; /** Socket file descriptor **/
			FILE * m_file; /** FILE wrapping m_sfd **/
			
			
	};
}
 
 #endif //_SOCKET_H
