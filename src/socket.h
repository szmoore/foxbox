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
	extern void HandleFlags();
	
	/**
	 * Represents a generic socket
	 * Can be written to or read from
	 */
	class Socket
	{
		protected:			
			Socket() : m_sfd(-1), m_file(NULL) {}
			void CopyFD(const Socket & cpy) {m_sfd = cpy.m_sfd; m_file = cpy.m_file;}
			
		public:
			Socket(const Socket & cpy) : m_sfd(cpy.m_sfd), m_file(cpy.m_file) {}
			Socket(FILE * file) : m_sfd(-1), m_file(file) {if (m_file != NULL) m_sfd = fileno(m_file);}
			virtual ~Socket() {this->Close();}
		
		public:
			/* NOTE: Use of virtual functions
		     * This is intentional. I am sacrificing some performance for ease of development.
		     * ie: The base functions work for either TCP sockets or file sockets
		     * 		But they won't work for WebSockets so I override them
		     * @see websocket.cpp
			 */
			
			virtual void Close();
			virtual bool Valid(); /** Socket can be read/written from/to **/
			
			virtual int GetRaw(void * buffer, size_t bytes); // read bytes into buffer
			virtual int SendRaw(const void * buffer, size_t bytes); // send buffer of size
				
			virtual bool GetToken(std::string & buffer, const char * delims = " \t\r\n", double timeout=-1, bool inclusive=false); /** Read until delimeter or timeout **/
			virtual bool Get(std::string & buffer, size_t bytes, double timeout = -1); /** Read number of characters or timeout **/
			
			inline bool Send(const std::string & buffer) {return SendRaw(buffer.c_str(),buffer.size());} /** Send C++ string **/
			virtual bool Send(const char * fmt, ...);
			int Dump(Socket & output, size_t block_size=BUFSIZ, double timeout=-1);


			/** Select first available for reading from **/
			static Socket * Select(const std::vector<Socket*> & sockets, std::vector<Socket*> * readable=NULL, double timeout=-1);

			static Socket * Select(size_t num_sockets, Socket * sockets);
			static Socket * Select(Socket * s1, ...);
			
			//TODO: Implement for writing as well?
			
			/** Implements cat ; in1->out1 and in2->out2 **/
			static std::pair<int, int> Cat(Socket & in1, Socket & out1, Socket & in2, Socket & out2, const char * delims = "\n", double timeout=-1);
			static std::pair<int, int> CatRaw(Socket & in1, Socket & out1, Socket & in2, Socket & out2, size_t block_size = 8, double timeout=-1);
			
			static int Compare(Socket & sock1, Socket & sock2, std::string * same = NULL, std::string * diff1 = NULL, std::string * diff2 = NULL, double timeout=-1);
			virtual bool CanReceive(double timeout=0);
			virtual bool CanSend(double timeout=0);
			
			/** wrapper to fwrite **/
			size_t Write(void * data, size_t size)
			{
				size_t result = fwrite(data, 1, size, m_file);
				return result;
			}
			/** wrapper to fread **/
			size_t Read(void * data, size_t size)
			{
				size_t result = fread(data, 1, size, m_file);
				return result;
			}
			
			/** DO NOT USE THIS (I had hoped to avoid it)**/
			int GetFD() const {return m_sfd;}  // @see websocket.cpp
			// tl;dr it is so WS::Socket can be used with Select
			
		protected:	
			friend class Pipe;
			
			int m_sfd; /** Socket file descriptor **/
			FILE * m_file; /** FILE wrapping m_sfd **/
			

	};
	
	class Pipe : public Socket
	{
		public:
			Pipe(FILE * input, FILE * output) : Socket(input), m_output(output) {}
			
			virtual int SendRaw(const void * buffer, size_t bytes) {return m_output.SendRaw(buffer, bytes);} // send buffer of size
			inline bool Send(const std::string & buffer) {return Send(buffer.c_str());} /** Send C++ string **/
			virtual bool Send(const char * fmt, ...);
			
		private:
			Socket m_output;
	};
	
	extern Pipe Stdio;
	
}
 #endif //_SOCKET_H
