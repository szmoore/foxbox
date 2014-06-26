

#include <openssl/sha.h>
#include "base64.h"

#include "websocket.h"


using namespace std;

namespace Foxbox {namespace WS
{
	
static string Magic(const string & key);

Server::Server(int port) : TCP::Server(port)
{

}

void Server::Listen()
{
	TCP::Server::Listen();
	HTTP::Request handshake;
	handshake.Receive(*this);
	map<string, string> & headers = handshake.Headers();
	auto i = headers.find("Sec-WebSocket-Key");
	if (i == headers.end())
	{
		Debug("No Sec-WebSocket-Key header!");
		m_valid = false;
		return;
	}
	string magic = Foxbox::WS::Magic(i->second);
	
	TCP::Socket::Send("HTTP/1.1 101 Switching Protocols\r\n");
	TCP::Socket::Send("Upgrade: WebSocket\r\n");
	TCP::Socket::Send("Connection: Upgrade\r\n");
	TCP::Socket::Send("Sec-WebSocket-Accept: %s\r\n", magic.c_str());
	TCP::Socket::Send("Sec-WebSocket-Protocol: %s\r\n\r\n", 
		headers["Sec-WebSocket-Protocol"].c_str());
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
	if (!Valid()) //Is the process running...
		return false; 

	va_list ap;
	va_start(ap, message);

	char buffer[BUFSIZ];
	int16_t len = vsprintf(buffer, message, ap);
	if (len < 0)
	{
		va_end(ap);
		Error("Error in vsprintf(3) - %s", strerror(errno));
		return false;
	}
	va_end(ap);
	
	// FIN, RSV1, RSV2, RSV3, OpCode
	fputc(0x81, m_file);
	char l = (len < 126) ? len : 126;
	//TODO: Deal with 64bit length
	// mask, payload length
	fputc(0x00 | l, m_file);
	if (l == 126)
	{
		fwrite(&len, sizeof(len), 1, m_file);
	}
	// masking key (no)
	//payload
	return (fwrite(buffer, len, sizeof(char), m_file) == (size_t)len); //TODO Error check
	
}



}} // end namespaces
