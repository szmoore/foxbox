/**
 * @file des.h
 * @brief Declarations for Data Encryption Standard (TDES and DES) as defined in FIPS 46-3
 */

#ifndef _3DES_H
#define _3DES_H

#include "socket.h"
#include <bitset>

namespace Foxbox
{
	namespace DES
	{
			std::bitset<64> EncryptBlock(const std::bitset<64> & input, const std::bitset<64> & key, bool decrypt=false);
			void EncryptString(const std::string & input, std::string & output, const std::bitset<64> & key, bool decrypt=false);
			void StringToHex(const std::string & input, std::string & output);
			class Socket : public Foxbox::Socket
			{
				public:
					Socket(Foxbox::Socket & socket, const std::bitset<64> & keyA, const std::bitset<64> & keyB, const std::bitset<64> & keyC);
					virtual ~Socket();
					
					// override virtual functions of Foxbox::Socket
					virtual int SendRaw(const void * buffer, size_t bytes);
					virtual bool Send(const char * message, ...);
					virtual bool GetToken(std::string & buffer, 
						const char * delims = " \t\r\n", double timeout=-1, 
						bool inclusive=false);
					virtual bool Get(std::string & buffer, size_t num_chars, double timeout = -1);
	
			
				private:
					/** Sendin/Receiving uses this socket **/
					
					std::bitset<64> m_keyA;
					std::bitset<64> m_keyB;
					std::bitset<64> m_keyC;
					char m_buffer[9];
					size_t m_buffer_index;
			};		
	}
}

#endif //_3DES_H
