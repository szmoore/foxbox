

#include <openssl/sha.h>
#include <climits>
#include "base64.h"

#include "websocket.h"


using namespace std;

namespace Foxbox {namespace WS
{
	
static string Magic(const string & key);




void Server::Listen()
{
	
	TCP::Server::Listen();
	Debug("Handshaking...");
	HTTP::Request handshake;
	handshake.Receive(*this);
	map<string, string> & headers = handshake.Headers();
	auto i = headers.find("Sec-WebSocket-Key");
	if (i == headers.end())
	{
		Debug("No Sec-WebSocket-Key header!");
		return;
	}
	string magic = Foxbox::WS::Magic(i->second);
	
	TCP::Socket::Send("HTTP/1.1 101 Switching Protocols\r\n");
	TCP::Socket::Send("Upgrade: WebSocket\r\n");
	TCP::Socket::Send("Connection: Upgrade\r\n");
	TCP::Socket::Send("Sec-WebSocket-Accept: %s\r\n", magic.c_str());
	TCP::Socket::Send("Sec-WebSocket-Protocol: %s\r\n\r\n", 
		headers["Sec-WebSocket-Protocol"].c_str());
	m_valid = true;
}

bool Server::Valid()
{
	m_valid &= TCP::Server::Valid();
	return m_valid;
}

string Magic(const string & key)
{										
	string result = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char hash[SHA_DIGEST_LENGTH];
	const unsigned char * d = (const unsigned char*)(result.c_str());
	SHA1(d, result.size(), hash);
	return base64_encode(hash, SHA_DIGEST_LENGTH);
}

bool Server::Send(const char * message, ...)
{
	va_list ap;
	va_start(ap, message);
	bool result = Protocol::Send(*this, 0, message, ap);
	va_end(ap);
	return result;
}

bool Client::Send(const char * message, ...)
{
	va_list ap;
	va_start(ap, message);
	bool result = Protocol::Send(*this, rand(), message, ap);
	va_end(ap);
	return result;
}

bool Protocol::Send(Socket & socket, int mask, const char * message, va_list ap)
{
	if (!socket.Valid()) 
		return false; 
		
	int size = vsnprintf(NULL, 0, message, ap);
	if (size < 0)
	{
		Error("Error in vsnprintf(3) - %s", strerror(errno));
		return false;
	}
	
	m_send_buffer.clear();
	m_send_buffer.resize(size+20, '\0');
	char * buffer = (char*)m_send_buffer.c_str(); // hooray for C strings

	int written = size+2;
	char * payload = buffer+2;
	buffer[0] = (char)0x81; // first and last frame, frame is text
	buffer[1] = (char)((mask != 0) ? 0x80 : 0x00);
	
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
		int32_t t(mask);
		memcpy(payload, &t, sizeof(t));
		payload += sizeof(t);
	}
	
	if (vsnprintf(payload, size, message, ap) < 0)
	{
		Error("Error in vsnprintf(3) - %s", strerror(errno));
		return false;
	}
	//Debug("Buffer is %s", buffer);
	//Debug("Payload is %s", payload);
	for (int i = 0; i < size && mask != 0; ++i)
	{
		payload[i] = payload[i] ^ *(payload-4+(i%4));
	}
	//Debug("Buffer is %s", buffer);
	//Debug("Payload is %s", payload);
	
	
	size_t w = socket.Write(buffer, (size_t)written);
	if (w != (size_t)written)
	{
		Error("Wrote %u instead of %lli bytes", w, written);
	}
	return (w == (size_t)written);
}

bool Server::GetMessage(string & buffer, double timeout)
{
	
	bool result = Protocol::GetMessage(*this, timeout);
	if (result)
	  buffer += Protocol::m_recv_buffer;
	return result;
}

bool Client::GetMessage(string & buffer, double timeout)
{
	bool result = Protocol::GetMessage(*this, timeout);
	if (result)
	  buffer += Protocol::m_recv_buffer;
	return result;
}

bool Protocol::GetMessage(Socket & socket, double timeout)
{
	
	if (!socket.CanReceive(timeout)) return false;
	
	uint8_t c = 0x00;
	
	int32_t mask = 0;
	int64_t size = 0;
	m_recv_buffer.clear();
	bool finished = false;
	while (!finished)
	{
		socket.Read(&c, 1); 
		finished = ((c & (1 << 7)) == 0x80);
		//Debug("start of frame is %.2x", c);
		if ((c & (1 << 0)) != 0x01) // check op code
		{
			Error("Only understand Text frames");
			return false;
		}
		
		
		socket.Read(&c, 1); // contains mask and payload length
		
		// Payload length
		size = (c & ~(1 << 7));
		
		if (size == 126)
		{
			// read next 2 bytes
			int16_t t(0);
			socket.Read(&t, sizeof(t));
			size = t;
		}
		else if (size == 127)
		{
			// read next 8 bytes
			size = 0;
			socket.Read(&size, sizeof(size));
		}
		//Debug("Mask + Payload %.2x, mask %.2x", c, (c & (1<<7)));
		//Debug("Payload %lli (%.2x)", size, (c & ~(1 << 7)));
		
		if ((c & (1 << 7)) == 0x80) // mask bit is set
		{
			socket.Read(&mask, sizeof(mask));
			
			//Debug("Mask set, mask is %u (%.4x)", mask, mask);
		}
		
		// read the payload
		unsigned old_size = m_recv_buffer.size();
		m_recv_buffer.resize(old_size + size);
		char * buffer = (char*)(m_recv_buffer.c_str() + old_size);
		socket.Read(buffer, size);
		//Debug("Buffer is %s", buffer);
		//Debug("Recv buf is %s", m_recv_buffer.c_str());
	}
	
	if (mask == 0)
	{
		//Debug("No mask, abort");
		return true;
	}
	char mask_str[4];
	memcpy(mask_str, &mask, sizeof(mask));
	for (unsigned i = 0; i < 4; ++i)
	{
		//Debug("Mask octec %d is %.2x", i,mask_str[i]);
	}
	
	for (unsigned i = 0; i < m_recv_buffer.size(); ++i)
	{
		m_recv_buffer[i] = m_recv_buffer[i] ^ mask_str[i%4];
	}
	return true;
}


}} // end namespaces
