

#include <openssl/sha.h>
#include "base64.h"

#include "websocket.h"


using namespace std;

namespace Foxbox {namespace WS
{
	
static string Magic(const string & key);

Server::Server(TCP::Socket & socket) : m_socket(socket), m_handshake(), m_valid(true)
{
	m_handshake.Receive(socket);
	map<string, string> & headers = m_handshake.Headers();
	auto i = headers.find("Upgrade");
	if (i == headers.end() || i->second != "websocket")
	{
		m_valid = false;
		return;
	}
	i = headers.find("Sec-WebSocket-Key");
	if (i == headers.end())
	{
		Debug("No Sec-WebSocket-Key header!");
		m_valid = false;
		return;
	}
	m_magic = Magic(i->second);
	
	m_socket.Send("HTTP/1.1 101 Switching Protocols\r\n");
	m_socket.Send("Upgrade: websocket\r\n");
	m_socket.Send("Connection: Upgrade\r\n");
	m_socket.Send("Sec-WebSocket-Accept: %s\r\n\r\n", m_magic.c_str());
}

bool Server::Valid()
{
	m_valid &= m_socket.Valid();
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

	
}} // end namespaces
