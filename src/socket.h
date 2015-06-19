/**
 * @file socket.h
 * @brief General Sockets - Declarations
 * @see socket.cpp - Definitions
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
	/**
	 * Represents a generic socket
	 * Can be written to or read from
	 */
	class Socket
	{
		protected:			
			Socket() : m_sfd(-1), m_file(NULL) {}
			
		public:
			Socket(const Socket & cpy) : m_sfd(cpy.m_sfd), m_file(cpy.m_file) {}
			Socket(FILE * file) : m_sfd(-1), m_file(file) {if (m_file != NULL) m_sfd = fileno(m_file);}
			virtual ~Socket() {if (m_file != NULL) fclose(m_file); m_file = NULL;}
		
		public:
			/* NOTE: Use of virtual functions
		     * This is intentional. I am sacrificing some performance for ease of development.
		     * ie: The base functions work for either TCP sockets or file sockets
		     * 		But they won't work for WebSockets so I override them
		     * @see websocket.cpp
			 */
			
			virtual void Close();
			virtual bool Valid(); /** Socket can be read/written from/to **/
			virtual bool Send(const char * print, ...);  /** Send formatted message **/
			virtual bool GetToken(std::string & buffer, const char * delims = " \t\r\n", double timeout=-1, bool inclusive=false); /** Read until delimeter or timeout **/
			virtual bool Get(std::string & buffer, unsigned num_chars, double timeout = -1); /** Read number of characters or timeout **/
			
			inline bool Send(const std::string & buffer) {return Send(buffer.c_str());} /** Send C++ string **/
			virtual int SendRaw(const void * buffer, unsigned size); // send raw data
			virtual int GetRaw(void * buffer, unsigned size); // get raw data
			
			
			inline bool GetToken(std::string & buffer, char delim, double timeout=-1) {return GetToken(buffer, ""+delim, timeout);}
			
			/** Select first available for reading from **/
			static Socket * Select(const std::vector<Socket*> & sockets, std::vector<Socket*> * readable=NULL);
			static Socket * Select(unsigned size, Socket * sockets);
			static Socket * Select(Socket * s1, ...);
			
			/** Implements cat ; in1->out1 and in2->out2 
			 * NOTE: in1 == in2 and out1 == out2 is allowed **/
			static void Dump(Socket & input, Socket & output, unsigned block_size=BUFSIZ);
			static void Cat(Socket & in1, Socket & out1, Socket & in2, Socket & out2);
			static void CatRaw(Socket & in1, Socket & out1, Socket & in2, Socket & out2, unsigned block_size=8);
			bool CanReceive(double timeout=-1);
			bool CanSend(double timeout=0);
			
			/** wrapper to fwrite **/
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
			/** wrapper to fread **/
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
			
			/** DO NOT USE THIS (I had hoped to avoid it)**/
			int GetFD() const {return m_sfd;}  // @see websocket.cpp
			// tl;dr it is so WS::Socket can be used with Select
			
		protected:	
			int m_sfd; /** Socket file descriptor **/
			FILE * m_file; /** FILE wrapping m_sfd **/
	};
}
 #endif //_SOCKET_H
