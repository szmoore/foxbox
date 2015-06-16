/**
 * @file http.cpp
 * @brief HTTP helper classes and functions - Definitions
 * @see http.h - Declarations
 * @see socket.h - General Socket base class
 * @see RFC 2616 http://www.w3.org/Protocols/rfc2616/rfc2616.html
 * 	NOTE: This is NOT fully RFC complaint
 */
 
#include "http.h"
#include "socket.h"
#include "process.h"

#include <sstream>

using namespace std;


									
 
namespace Foxbox {namespace HTTP
{
	
Request::Request(const string & hostname, const string & request_type, 
				 const string & query) 
	: m_hostname(hostname), m_request_type(request_type), m_query(query),
		m_path(query), m_params(), m_cookies(), m_headers(),
		m_split_path(NULL), m_valid(false)
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
	m_valid = false;
	m_request_type.clear();
	m_path.clear();
	m_params.clear();
	m_cookies.clear();
	
	if (!socket.Valid()) return false;
	
	
	socket.GetToken(m_request_type, " ");
	//Debug("Request type... %s", m_request_type.c_str());
	socket.GetToken(m_query, " ");
	//Debug("Path+Query... %s", m_query.c_str());
	
	string protocol("");
	socket.GetToken(protocol, "\n");
	strip(protocol);
	//Debug("Protocol... %s", protocol.c_str());
	if (protocol != "HTTP/1.1")
		return false;
	strip(m_request_type);
	strip(m_query);

	m_path = ParseQuery(m_params, m_query, '?', '&', '=', " \r\n:;");
	strip(m_path, "/");
	string garbage("");
	socket.GetToken(garbage, "\n");
	strip(garbage);
	while (socket.Valid() && socket.CanReceive(0))
	{
		garbage.clear();
		socket.GetToken(garbage, "\n");
		strip(garbage);
		//Debug("Line is \"%s\"", garbage.c_str());
		string header("");
		string value("");
		stringstream s(garbage);
		getline(s, header, ':');
		if (s.good())
			getline(s, value);
		strip(header);
		strip(value);
		if (header == "Cookie")
		{
			ParseQuery(m_cookies, value, '\0', ';','=', " \r\n:\t");
		}
		else if (header.size() > 0)
		{
			m_headers[header] = value;
		}
	}
	m_valid = true;
	return true;
}

unsigned ParseResponseHeaders(Socket & socket, map<string, string> * headers, string * reason)
{
	if (!socket.Valid() || !socket.CanReceive(1))
	{
		Error("Socket not valid");
		return 0;
	}
	
	string line("");
	socket.GetToken(line, " ");
	strip(line);
	if (line != "HTTP/1.1")
	{
		Error("Got \"%s\", expected HTTP/1.1", line.c_str());
		return 0;
	}
	line.clear();
	socket.GetToken(line, " ");
	strip(line);
	unsigned code; 
	stringstream s(line);  s >> code;
	
	line.clear();
	socket.GetToken(line, "\n");
	strip(line);
	if (reason != NULL)
		*reason = line;
		
	while (socket.Valid() && socket.CanReceive(0))
	{
		line.clear();
		socket.GetToken(line, "\n");
		strip(line);
		string header("");
		string value("");
		
		stringstream s(line);
		getline(s, header, ':');
		if (s.good())
			getline(s, value);
		
		strip(header);
		strip(value);
		//Debug("Header %s, Value %s", header.c_str(), value.c_str());
		if (header.size() == 0 && value.size() == 0)
			break;
		if (headers != NULL && header.size() != 0)
		{
			headers->operator[](header) = value;
		}
	}
	
	return code;
	
}

bool Request::Send(Socket & socket)
{
	m_valid = false;
	if (!socket.Valid()) return false;
	
	strip(m_query, "/");
	
	socket.Send("GET /%s HTTP/1.1\r\n", m_query.c_str());
	m_headers["Host"] = m_hostname;
	m_headers["User-Agent"] += " Foxbox/1.0";
	m_headers["Accept"] += " */*";
	m_headers["Connection"] += " Keep-Alive";
	
	for (auto & it : m_headers)
	{
		strip(it.second);
		socket.Send("%s: %s\r\n", it.first.c_str(), it.second.c_str());
	}
	socket.Send("\r\n");
	
	m_valid = socket.Valid();
	return m_valid;
}

const char * StatusMessage(unsigned code)
{
	switch (code)
	{
		case 200:
			return "OK";
		case 404:
			return "Not found";
		case 400:
			return "Bad Request";
		default:
			return "?";
	}
}


bool SendPlain(Socket & socket, unsigned status, const char * message)
{
	return (socket.Send("HTTP/1.1 %u %s\r\n", status, StatusMessage(status))
		&& socket.Send("Content-Type: text/plain; charset=utf-8\r\n\r\n")
		&& socket.Send(message));
}

bool SendJSON(Socket & socket, const map<string, string> & m, unsigned status)
{
	bool result = true;
	if (status != 0)
	{
		result &= (socket.Send("HTTP/1.1 %u %s\r\n",status,StatusMessage(status))
			&& socket.Send("Content-Type: application/json; charset=utf-8\r\n\r\n{\n"));
	}
	else
	{
		result &= socket.Send("{\n");
	}
	for (auto i = m.begin(); i != m.end(); ++i)
	{
		if (i != m.begin())
			result &= socket.Send(",\n");
		result &= socket.Send("\t\"%s\" : \"%s\"", i->first.c_str(), i->second.c_str());
	}
	return (socket.Send("\n}\n") && result);
}

bool SendFile(Socket & socket, const char * filename, unsigned status)
{
	FILE * file = fopen(filename, "r");
	if (file == NULL)
	{
		if (status != 0)
		{
			socket.Send("HTTP/1.1 404 Not found\r\n");
			socket.Send("Content-Type: text/plain; charset=utf-8\r\n\r\n");
			socket.Send("File \"%s\" not found.\n", filename);
		}
		return false;
	}
	bool result = socket.Send("HTTP/1.1 %u %s\r\n", status, StatusMessage(status));
	if (status != 0)
	{	
		// guess the content type from the file extension (if present)
		char * extension = strrchr((char*)filename, (int)'.');
		extension = (extension == NULL) ? (char*)"" : extension;
		result &= socket.Send("HTTP/1.1 %u %s\r\n", status, StatusMessage(status));
		const char * type = "Content-Type: text/plain";
		if (strcmp(extension, ".html") == 0)
		{
			type = "Content-Type: text/html";
			result &= socket.Send("%s; charset=utf-8\r\n\r\n", type);
		}
		else if (strcmp(extension, ".svg") == 0)
		{
			type = "Content-Type: image/svg+xml";
			result &= socket.Send("%s; charset=utf-8\r\n\r\n", type);
		}
		else if (strcmp(extension, ".png") == 0)
		{
			type = "Content-Type: image/png";
			result &= socket.Send("%s\r\n\r\n", type);
		}
		else if (strcmp(extension, ".gif") == 0)
		{
			type = "Content-Type: image/gif";
			result &= socket.Send("%s\r\n\r\n", type);
			
		}
		else
		{
			result &= socket.Send("%s; charset=utf-8\r\n\r\n", type);
		}
		//TODO: Support other file types

		
	}

	if (result)
	{
		Socket input(file);
		string line("");
		Socket::Dump(input, socket);
	}
	/*
	while (input.Valid())
	{
		line.clear();
		input.GetToken(line, "\n", -1, true);
		result &= socket.Send(line);
	}
	*/
	return result;
}

void Request::CGI(TCP::Socket & socket, const char * program, const map<string, string> & env)
{

	try
	{
		//setup environment variables
		map<string, string> cgi_env;
	
		// currently just setting what Apache/2.2.22 sets
		cgi_env["SERVER_SOFTWARE"] = "Foxbox";
		cgi_env["SCRIPT_NAME"] = "/"+string(program);
		cgi_env["SERVER_SIGNATURE"] = "Foxbox Server https://github.com/szmoore/foxbox";
		cgi_env["REQUEST_METHOD"] = m_request_type;
		cgi_env["SERVER_PROTOCOL"] = "HTTP/1.1";
		cgi_env["QUERY_STRING"] = m_query;
		cgi_env["HTTP_USER_AGENT"] = m_headers["User-Agent"];
		cgi_env["SERVER_NAME"] = "";
		cgi_env["REMOTE_ADDR"] = socket.RemoteAddress();
	
		// use the additional env provided (overwrite any defaults).
		for (map<string,string>::const_iterator it = env.begin(); it != env.end(); ++it)
		{
			cgi_env[it->first] = cgi_env[it->second];
		}
	
		Process proc(program, cgi_env);
		Socket::Cat(socket, proc, proc, socket);
	}
	catch (Exception e)
	{
		HTTP::SendPlain(socket, 500, "An error occured executing a CGI program. Check the server logs for more details.");
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
	//Debug("Called");
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
