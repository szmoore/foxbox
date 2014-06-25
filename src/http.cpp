/**
 * @file http.cpp
 * @brief Implementation of HTTP
 */
 
#include "http.h"
#include "socket.h"

#include <sstream>

using namespace std;
 
namespace Foxbox {namespace HTTP
{
	
Request::Request(const string & hostname, const string & request_type, 
				 const string & query) 
	: m_hostname(hostname), m_request_type(request_type), m_query(query),
		m_path(query), m_params(), m_cookies(), m_split_path(NULL)
{
	m_path = ParseQuery(m_params, m_path, '?', '&', '=', " \r\n:;");
}

Request::~Request()
{
	delete m_split_path;
}

vector<string> & Request::SplitPath(char delim)
{
	if (m_split_path != NULL) return *(m_split_path);
	m_split_path = new vector<string>();
	stringstream s(m_path);
	string token("");
	while (s.good())
	{
		token.clear();
		getline(s, token, delim);
		if (token.size() > 0)
			m_split_path->push_back(token);
	}
	if (m_split_path->size() == 0)
		m_split_path->push_back("");
	return *(m_split_path);
		
}
	
bool Request::Receive(Socket & socket)
{
	m_request_type.clear();
	m_path.clear();
	m_params.clear();
	m_cookies.clear();
	
	if (!socket.Valid()) return false;
	
	Debug("Request type...");
	socket.GetToken(m_request_type);
	Debug("Reuest path...");
	socket.GetToken(m_query);

	m_path = ParseQuery(m_params, m_query, '?', '&', '=', " \r\n:;");
	while (m_path.size() > 0 && strchr("/", m_path.front()) != NULL) m_path.erase(m_path.begin());
	string garbage;
	bool got_cookie = false;
	while (!got_cookie && socket.CanReceive(0))
	{
		Debug("Search for cookie");
		garbage.clear();
		socket.GetToken(garbage);
		got_cookie = (garbage == "Cookie:");
	} 
	
	if (got_cookie)
	{
		garbage.clear();
		socket.GetToken(garbage, "\n");
		Debug("Cookie string is %s", garbage.c_str());
		ParseQuery(m_cookies, garbage, '\0', ';', '=', " \r\n:");
	}
	
	// Ignore anything else
	while (socket.CanReceive(0)) 
	{
		Debug("Getting garbage");
		socket.GetToken(garbage, "\n");
		garbage.clear();
	}
	return true;
}

bool Request::Send(Socket & socket)
{
	if (!socket.Valid()) return false;
	socket.Send("GET /%s HTTP/1.1\n", m_query.c_str());
	socket.Send("Host: %s\n", m_hostname.c_str());
	socket.Send("User-Agent: Foxbox/1.0 Wget/1.15\n"); // lies
	socket.Send("Accept: */*\n");
	socket.Send("Connection: Keep-Alive\n\n");
	return socket.Valid();
}

void SendJSON(Socket & socket, const map<string, string> & m, bool with_header)
{
	if (with_header)
	{
		socket.Send("HTTP/1.1 200 OK\r\n");
		socket.Send("Content-Type: application/json; charset=utf-8\r\n\r\n{\n");
	}
	for (auto i = m.begin(); i != m.end(); ++i)
	{
		if (i != m.begin())
			socket.Send(",\n");
		socket.Send("\t\"%s\" : \"%s\"", i->first.c_str(), i->second.c_str());
	}
	socket.Send("\n}\n");
}

void SendFile(Socket & socket, const string & filename, bool with_header)
{
	FILE * file = fopen(filename.c_str(), "r");
	if (file == NULL)
	{
		if (with_header)
		{
			socket.Send("HTTP/1.1 404 Not found\r\n");
			socket.Send("Content-Type: text/plain; charset=utf-8\r\n\r\n");
			socket.Send("File \"%s\" not found.\n", filename.c_str());
		}
		return;
	}
	
	if (with_header)
	{	
		char * extension = strchr((char*)filename.c_str(), (int)'.');
		socket.Send("HTTP/1.1 200 OK\r\n");
		if (extension == NULL)
		{
			socket.Send("Content-Type: text/plain");
		}
		else if (strcmp(extension, ".html") == 0)
		{
			socket.Send("Content-Type: text/html");
		}
		else
		{
			socket.Send("Content-type: text/plain");
		}
		socket.Send("; charset=utf-8\r\n\r\n");
	}

	
	Socket input(file);
	string line("");
	while (input.Valid())
	{
		line.clear();
		input.GetToken(line, "\n", -1, true);
		socket.Send(line);
	}
}

void FormQuery(string & s, const map<string, string> & m, char seperator, char equals)
{
	for (auto i = m.begin(); i != m.end(); ++i)
	{
		if (i != m.begin())
			s += seperator;
		s += i->first;
		s += equals;
		s += i->second;
	}
}

string ParseQuery(map<string, string> & m, const string & s, char start, 
				char seperator, char equals, const char * strip)
{
	stringstream ss(s);
	string ignored("");
	while (start != '\0' && ss.good())
	{
		int c = ss.get();
		if (c == EOF || (char)c == start) break;
		ignored += (char)c;
	}
	while (ss.good())
	{
		string kv("");
		getline(ss, kv, seperator);
		while (kv.size() > 0 && strchr(strip, kv.front()) != NULL) kv.erase(kv.begin());
		while (kv.size() > 0 && strchr(strip, kv.back()) != NULL) kv.pop_back();
		
		stringstream kvs(kv);
		if (!kvs.good()) continue;
	
		string key("");
		string value("");
		getline(kvs, key, equals);
		if (kvs.good())
			getline(kvs, value, seperator);
		Debug("%s = %s", key.c_str(), value.c_str());
		m[key] = value;	
	}
	return ignored;
}
	
}} // end namespaces
