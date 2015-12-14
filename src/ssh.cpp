#include "ssh.h"

using namespace std;

namespace Foxbox {namespace SSH
{

Client::Client(char * addr, int port) : Socket(m_client), m_client(addr, port)
{
	// read version string from server
	string buffer;
	m_client.GetToken(buffer, "\n");
	
	// send version string to server
	m_client.Send("SSH-2.0-Foxbox_1.0\r\n");
	
}


}}
