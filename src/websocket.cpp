/**
 * @file websocket.cpp
 * @brief WebSocket Protocol using TCP::Sockets - Definitions
 * @see websocket.h - Declarations
 * @see tcp.h - TCP::Sockets
 * @see RFC-6455 http://tools.ietf.org/html/rfc6455 
 * NOTE: This is NOT fully RFC complaint
 */

#include <climits>
#include <map>
#include <ctime>

#include "base64.h"
#include "sha1.h"
#include "websocket.h"


using namespace std;

namespace Foxbox {namespace WS
{
	
static string Magic(const string & key);

Socket::Socket(TCP::Socket & tcp_socket, bool use_mask)
	: Foxbox::Socket(), m_tcp_socket(tcp_socket), m_use_mask(use_mask), 
	  m_valid(false), m_send_buffer(""), m_recv_buffer(""),
	  m_recv_tokeniser("")
{
	Foxbox::Socket::CopyFD(m_tcp_socket);
}

Client::Client(const char * server_addr, int port, const char * query, const char * proto) 
	: WS::Socket(m_client), m_client(server_addr, port)
{
	srand(time(NULL));
	HTTP::Request handshake(server_addr, "GET", query);
	
	// The RFC says nothing obvious about how this key should be chosen
	// So I just picked the same key given in the RFC.
	// Iceweasel appears to make different keys for each connection.
	string key("dGhlIHNhbXBsZSBub25jZQ==");
	string magic(Magic(key));
	
	HTTP::Update(handshake.Headers(), {
		{"Connection", "Upgrade"},
		{"Sec-WebSocket-Key", key},
		{"Sec-WebSocket-Protocol", proto},
		{"Upgrade", "websocket"},
		{"Sec-WebSocket-Version", "13"}
	});
	handshake.Send(m_client);
	if (!m_client.Valid())
		return;

	
	map<string, string> headers;
	//Debug("Waiting on response headers.");
	if (HTTP::ParseResponseHeaders(m_client, &headers, NULL) != 101)
	{
		Error("Invalid server handshake");
		return;
	}
	//Debug("Got response headers.");
	auto i = headers.find("Sec-WebSocket-Accept");
	if (i == headers.end())
	{
		//Error("No \"Sec-WebSocket-Accept\" header");
		Foxbox::Socket s(stderr);
		HTTP::SendJSON(s, headers, false);
		return;	
	}
		
	if (i->second != magic)
	{
		Warn("Magic string \"%s\" does not match \"%s\" (key \"%s\")",
			i->second.c_str(), magic.c_str(), key.c_str());
	}
	//Debug("Handshake on client done.");
	m_valid = true;
}

Server::Server(int port) : WS::Socket(m_server), m_server(port)
{
	
}

Server::Server(const Server & cpy) : WS::Socket(m_server), m_server(cpy.m_server.Port())
{
	
}

bool Server::Listen()
{
	m_valid = false;
	if (!m_server.Listen())
		return false;
	//Debug("Handshaking...");
	HTTP::Request handshake;
	if (!handshake.Receive(m_server))
	{
		Error("Failed to process HTTP request");
	}
	map<string, string> & headers = handshake.Headers();
	auto i = headers.find("Sec-WebSocket-Key");
	if (i == headers.end())
	{
		Error("No Sec-WebSocket-Key header!");
		for (i = headers.begin(); i != headers.end(); ++i)
		{
			Error("Key: %s = %s", i->first.c_str(), i->second.c_str());
		}
		return false;
	}
	//Debug("Finished handshake");
	string magic = Foxbox::WS::Magic(i->second);
	
	m_server.Send("HTTP/1.1 101 Switching Protocols\r\n");
	m_server.Send("Upgrade: WebSocket\r\n");
	m_server.Send("Connection: Upgrade\r\n");
	m_server.Send("Sec-WebSocket-Accept: %s\r\n", magic.c_str());
	m_server.Send("Sec-WebSocket-Protocol: %s\r\n\r\n", 
		headers["Sec-WebSocket-Protocol"].c_str());
	m_valid = true;
	return true;
}

bool Socket::Valid()
{
	//Debug("Called! Valid is %d and m_tcp_socket.Valid() is %d", m_valid, m_tcp_socket.Valid());
	m_valid &= m_tcp_socket.Valid();
	m_sfd = m_tcp_socket.GetFD();
	return m_valid;
}

string Magic(const string & key)
{										
	string result = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char hash[SHA1HashSize];
	const unsigned char * d = (const unsigned char*)(result.c_str());
	//SHA1(d, result.size(), hash);
	SHA1Context sha;
	SHA1Reset(&sha);
	SHA1Input(&sha, d, result.size());
	SHA1Result(&sha, hash);	
	return base64_encode(hash, SHA1HashSize);
}

bool Socket::Send(const char * message, ...)
{
	if (!m_tcp_socket.Valid()) 
		return false; 
		
	int32_t mask = (m_use_mask) ? rand() : 0;
	uint8_t mask_str[4];
	memcpy(mask_str+0, &mask, sizeof(mask));
	
	va_list ap;
	va_start(ap, message);
		
	//Debug("Message is \"%s\"", message);
		
	int size = vsnprintf(NULL, 0, message, ap);
	va_end(ap);
	if (size < 0)
	{
		
		Error("Error in vsnprintf(3) - %s", StrError(errno));
		return false;
	}
	
	uint8_t * buffer = new uint8_t[size+20];
	
	int written = size+2;
	uint8_t * payload = buffer+2;
	buffer[0] = 0x81; // first and last frame, opcode of frame is text
	buffer[1] = ((mask != 0) ? 0x80 : 0x00);
	
	if (size < 126) 
	{
		buffer[1] |= size;
		//Debug("Short message %s", message);
	}
	else if (size < SHRT_MAX)
	{
		buffer[1] |= 126;
		int16_t t(size);
		memcpy(payload, &t, sizeof(t)); 
		payload += sizeof(t);
		written += sizeof(t);
	}
	else if (size < LLONG_MAX)
	{
		buffer[1] |= 127;
		int64_t t(size);
		memcpy(payload, &t, sizeof(t));
		payload += sizeof(t);
		written += sizeof(t);
	}
	//Debug("Buffer[1] is %u (size is %d)", (uint8_t)buffer[1], size);
	if (mask != 0)
	{
		*(payload++) = mask_str[0];
		*(payload++) = mask_str[1];
		*(payload++) = mask_str[2];
		*(payload++) = mask_str[3];
		written += 4;
	}
	va_list ap2;
	va_start(ap2, message);
	if (vsnprintf((char*)payload, size+1, message, ap2) < 0)
	{
		va_end(ap2);
		Error("Error in vsnprintf(3) - %s", StrError(errno));
		return false;
	}
	va_end(ap2);
	//Debug("Buffer is %s", buffer);
	//Debug("Payload is %s", payload);
	if (mask != 0)
	{
		
		
		//Debug("mask is %.8x", mask);
		for (int i = 0; i < size; ++i)
		{
			//Debug("Payload[%d] is %c %.4x and mask[%d] is %.4x", i, payload[i], payload[i], i%4,mask_str[i%4]);
			payload[i] = (uint8_t)(payload[i] ^ (uint8_t)(mask_str[i%4]));
			//Debug("Payload[%d] is %.4x and EOF is %.4x", i, payload[i], EOF);
		}
	}
	//Debug("Buffer is %s", buffer);
	//Debug("Payload is %s", payload);
	//Debug("Write %d bytes, %d in payload", written, size);
	
	size_t w = m_tcp_socket.Write(buffer, (size_t)written*sizeof(uint8_t));
	if (w != (size_t)written)
	{
		Error("Wrote %u instead of %lli bytes", w, written);
	}
	delete [] buffer;
	return (w == (size_t)written);
}

bool Socket::GetMessage(string & buffer, double timeout)
{
	
	bool result = GetMessage(timeout);
	//Debug("Buffer %s, result %d", m_recv_buffer.c_str(), result);
	if (result)
	  buffer += m_recv_buffer;
	return result;
}

bool Socket::GetMessage(double timeout)
{
	
	if (!m_tcp_socket.CanReceive(timeout)) return false;
	
	uint8_t c = 0x00;
	
	int32_t mask = 0;
	int64_t size = 0;
	m_recv_buffer.clear();
	m_recv_tokeniser.str("");
	bool finished = false;
	while (!finished)
	{
		m_tcp_socket.Read(&c, sizeof(uint8_t)); 
		finished = ((c & (1 << 7)) == 0x80);
//		Debug("start of frame is %.2x", c);
		if ((c & (1 << 0)) != 0x01) // check op code
		{
			//Error("Only understand Text frames");
			return false;
		}
		
		
		m_tcp_socket.Read(&c, 1); // contains mask and payload length
		
		// Payload length
		size = (c & ~(1 << 7));
		
		if (size == 126)
		{
			// read next 2 bytes
			int16_t t(0);
			m_tcp_socket.Read(&t, sizeof(t));
			size = t;
		}
		else if (size == 127)
		{
			// read next 8 bytes
			size = 0;
			m_tcp_socket.Read(&size, sizeof(size));
		}
		//Debug("Mask + Payload %.2x, mask %.2x", c, (c & (1<<7)));
		//Debug("Payload %lli (%.2x)", size, (c & ~(1 << 7)));
		
		if ((c & (1 << 7)) == 0x80) // mask bit is set
		{
			m_tcp_socket.Read(&mask, sizeof(mask));
			
			//Debug("Mask set, mask is %u (%.4x)", mask, mask);
		}
		
		// read the payload
		//TODO: Optimise memory allocations
		uint8_t * buffer = new uint8_t[size+1];
		size_t read = m_tcp_socket.Read(buffer, size*sizeof(uint8_t));
		buffer[read] = 0;
		if (read != (size_t)size)
		{
			Warn("Didn't read all bytes; read %u instead of %d", read, size);
		}
		
		unsigned old_size = m_recv_buffer.size();
		m_recv_buffer.reserve(old_size + size);
		
		
		uint8_t mask_str[4];
		memcpy(mask_str, &mask, sizeof(mask));
		
		for (unsigned i = 0; i < size; ++i)
		{
			if (mask != 0)
				buffer[i] ^= mask_str[i%4];
			m_recv_buffer += (char)(buffer[i]);
		}
		delete [] buffer;

	}
	//Debug("Got message %s", m_recv_buffer.c_str());
	m_recv_tokeniser.str(m_recv_buffer);
	m_recv_tokeniser.clear();
	return true;
}

bool Socket::GetToken(string & buffer, const char * delims, double timeout, bool inclusive)
{
	//Debug("Tokenise \"%s\"", m_recv_tokeniser.str().c_str());
	if (!m_recv_tokeniser.good())
	{
		//Debug("Not good...");
		if (!GetMessage(timeout))
			return false;
	}
	//Debug("Tokenise \"%s\"", m_recv_tokeniser.str().c_str());
	
	
	int c = m_recv_tokeniser.get();
	if (!inclusive)
	{
		if (!m_recv_tokeniser.good() && strchr(delims, c) != NULL)
		{
			//Debug("Empty.");
			return false;
		}
	}
	//Debug("At token. Tokeniser is %d, last char is \"%c\"", m_recv_tokeniser.good(), (char)c);
	while (m_recv_tokeniser.good() && strchr(delims, c) == NULL)
	{
		//Debug("Got char %c", (char)c);
		//Debug("Got char %c", (char)c);
		buffer += c;
		c = m_recv_tokeniser.get();
	}
	if (m_recv_tokeniser.good() && inclusive)
		buffer += c;
	return (c != EOF);
}


bool Socket::Get(string & buffer, unsigned num_chars, double timeout)
{
	for (unsigned i = 0; i < num_chars; ++i)
	{
		if (!m_recv_tokeniser.good())
		{
			if (!GetMessage(timeout))
				return false;
		}
		int c = m_recv_tokeniser.get();
		if (c == EOF)
			return false;
		buffer += c;
	}
	return true;
}

}} // end namespaces
