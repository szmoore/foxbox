/**
 * @file socket.cpp
 * @brief General Sockets - Definitions
 * @see socket.h - Declarations
 * NOTE: Wraps to POSIX sockets
 */

#include "socket.h"
 
using namespace std; 

namespace Foxbox
{
	
bool Socket::Valid()
{
	//TODO: TIDY
	if (m_sfd == -1) return false;
	if (m_file == NULL) 
	{
		m_file = fdopen(m_sfd, "r+");
		if (m_file == NULL)
		{
			m_sfd = -1;
			return false;
		}
		setbuf(m_file, NULL);
	}
	if (feof(m_file) != 0 || ferror(m_file) != 0)
	{
		close(m_sfd);
		m_sfd = -1;
		m_file = NULL;
		return false;
	}
	return true;
}

void Socket::Close()
{
	if (Valid()) 
	{
		close(m_sfd); 
		m_sfd = -1;
		m_file = NULL;
	}
}

/** Select the first available Socket in v for Reading
 * @param v Socket's to Select from
 * @returns Socket* in v which can be read from
 * 	If more than one can be read from, returns the first
 *  If none can be read from, returns NULL
 */
Socket * Socket::Select(const vector<Socket*> & v, vector<Socket*> * readable)
{
	int max_sfd = 0;
	fd_set readfds;
	FD_ZERO(&readfds);
	for (unsigned i = 0; i < v.size(); ++i)
	{
		if (v[i]->m_sfd > max_sfd) max_sfd = v[i]->m_sfd;
		FD_SET(v[i]->m_sfd, &readfds);
	}
	
	//struct timeval tv = {0,0};
	
	if (select(max_sfd+1, &readfds, NULL, NULL, NULL) < 0)
	{
		if (errno != EINTR)
			Error("Error in select - %s", strerror(errno));
		return NULL;
	}
		for (unsigned i = 0; i < v.size(); ++i)
	{
		if (FD_ISSET(v[i]->m_sfd, &readfds))
		{
			if (readable == NULL)
				return v[i];
			else
				readable->push_back(v[i]);
		}
	}
	if (readable != NULL && readable->size() > 0)
		return readable->at(0);
	return NULL;
}

/**
 * Wrap to Select given a C style array 
 * @param size Size of the array
 * @param sockets Array
 */
Socket * Socket::Select(unsigned size, Socket * sockets)
{
	// Could probably do this accessing v.data() somehow ?
	vector<Socket*> v(size);
	for (unsigned i = 0; i < size; ++i)
		v[i] = sockets+i;
	return Select(v);
}

/**
 * Wrap to Select given va_args
 * @param s1 First Socket
 * @param ... additional Sockets
 */
 Socket * Socket::Select(Socket * s1, ...)
 {
	 va_list va;
	 va_start(va, s1);
	 vector<Socket*> v;
	 Socket * s = s1;
	 while (s != NULL)
	 {
		 v.push_back(s);
		 s = va_arg(va, Socket*);
	 }
	 va_end(va);
	 return Socket::Select(v);
 }
 
 /**
 * Send formatted string over socket
 * @param print - Format string
 * @param va_args - Format arguments
 * @returns true on success, false on error (and prints error message)
 */
bool Socket::Send(const char * print, ...)
{
	if (!Valid()) //Is the process running...
		return false; 

	va_list ap;
	va_start(ap, print);

	if (vfprintf(m_file, print, ap) < 0)
	{
		va_end(ap);
		Error("Error in vfprintf(3) - %s", strerror(errno));
		return false;
	}
	va_end(ap);
	return true;
}

int Socket::SendRaw(const void * buffer, unsigned size)
{
	if (!Valid())
		return false;
	errno = 0;
	int written = write(m_sfd, buffer, size);
	if (written < 0 || errno != 0)
	{
		Error("Wrote %u bytes, not %u - %s", written, size, strerror(errno));
		return written;
	}
	return written;
}

int Socket::GetRaw(void * buffer, unsigned size)
{
	if (!Valid())
		return false;
	errno = 0;
	int received = read(m_sfd, buffer, size);
	if (received < 0 || errno != 0)
	{
		Error("Read %u bytes, not %u - %s", received, size, strerror(errno));
		return received;
	}
	// we are at the end of file
	// feof() on m_file will fail here, so close ourselves instead of relying on Valid :S
	// not supposed to mix stream and file descriptor functions...
	if (received == 0)
		Close();
	return received;
}

bool Socket::CanReceive(double timeout)
{
	if (!Valid())
	{
		Error("Socket is not valid.");
		return false;
	}

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(m_sfd, &readfds);
	
	struct timeval tv;
	tv.tv_sec = (int)(timeout);
	tv.tv_usec = (timeout - (double)((int)timeout)) * 1000000;
	
	int err = 0;
	if (timeout >= 0)
		err = select(m_sfd+1, &readfds, NULL, NULL, &tv);
	else
		err = select(m_sfd+1, &readfds, NULL, NULL, NULL);
	
	if (err < 0)
	{
		Error("Error in select - %s", strerror(errno));
		return false;
	}

	if (!FD_ISSET(m_sfd, &readfds))
	{
		//Warn("Timed out after %.2f seconds", timeout);
		return false; //Timed out
	}
	return true;
}

/**
 * Receives a fixed number of characters from the Socket, with optional timeout
 * @param buffer - C++ std::string to store the resultant message in
 * @param num_chars - Number of characters to read
 * @param timeout - If >0, maximum time to wait before returning failure. If <0, will wait indefinitely
 * @returns true if successful, false if the timeout occured (prints warning) or an error occured (prints error)
 */
bool Socket::Get(string & buffer, unsigned num_chars, double timeout)
{
	if (!Valid()) return false;
	if (!CanReceive(timeout)) return false;
	int c; unsigned i = 0;
	for (c = fgetc(m_file); (i++ < num_chars && c != EOF); c = fgetc(m_file))
	{	
		buffer += c;
	}
	if (c == EOF)
	{
		close(m_sfd); m_sfd = -1;
	}
	return (c != EOF);
}

/**
 * Get a token from the Socket, with optional timeout
 * @param buffer - C++ std::string to store the resultant message in
 * @param delims - Delimiters for messages (default '\n')
 * @param timeout - If >0, maximum time to wait before returning failure. If <0, will wait indefinitely
 * @param inclusive - If true, delimiters will be included
 * @returns true if successful, false if the timeout occured (prints warning) or an error occured (prints error)
 */
bool Socket::GetToken(string & buffer, const char * delims, double timeout, bool inclusive)
{
	if (!Valid()) return false;
	if (!CanReceive(timeout)) return false;
	int c = fgetc(m_file);
	if (!inclusive)
	{
		if (c == EOF && strchr(delims, c) != NULL)
			return false;
	}
	while (c != EOF && strchr(delims, c) == NULL)
	{	
		buffer += c;
		c = fgetc(m_file);
	}
	if (c == EOF)
	{
		close(m_sfd); m_sfd = -1;
	}
	else if (inclusive)
	{
		buffer += c;
	}
	return (c != EOF);
}

void Socket::Dump(Socket & input, Socket & output, unsigned block_size)
{
	char * buffer = new char[block_size];
	while (input.Valid() && output.Valid())
	{
		int read = input.GetRaw(buffer, block_size);
		if (read > 0)
			output.SendRaw(buffer, read);
	}
	delete [] buffer;
}

void Socket::CatRaw(Socket & in1, Socket & out1, Socket & in2, Socket & out2, unsigned block_size)
{
	vector<Socket*> input(2);
	input[0] = &in1;
	input[1] = &in2;
	vector<Socket*> readable;
	
	char * buffer = new char[block_size];
	
	while (in1.Valid() && in2.Valid() && out1.Valid() && out2.Valid())
	{
		readable.clear();
		Socket::Select(input, &readable);
		for (auto it = readable.begin(); it != readable.end(); ++it)
		{
			Socket * in = *it;
			Socket * out = (in == &in1) ? &out1 : &out2;
			//Debug("%s", (in == &in1) ? "ONE" : "TWO");
			string s("");
			in->GetRaw(buffer, block_size);
			//Debug("Got s = %s", s.c_str());
			out->SendRaw(buffer, block_size);
		}
	}
	delete [] buffer;
}

void Socket::Cat(Socket & in1, Socket & out1, Socket & in2, Socket & out2)
{
	vector<Socket*> input(2);
	input[0] = &in1;
	input[1] = &in2;
	vector<Socket*> readable;
	while (in1.Valid() && in2.Valid() && out1.Valid() && out2.Valid())
	{
		readable.clear();
		Socket::Select(input, &readable);
		for (auto it = readable.begin(); it != readable.end(); ++it)
		{
			Socket * in = *it;
			Socket * out = (in == &in1) ? &out1 : &out2;
			//Debug("%s", (in == &in1) ? "ONE" : "TWO");
			string s("");
			in->GetToken(s, "\n",-1,true);
			//Debug("Got s = %s", s.c_str());
			out->Send(s);
		}
	}
	
	/*
	if (!in1.Valid())
		Debug("in1 became invalid!");
	if (!in2.Valid())
		Debug("in2 became invalid!");
	if (!out1.Valid())
		Debug("out1 became invalid!");
	if (!out2.Valid())
		Debug("out2 became invalid!");
	*/
}

} //end namespace

