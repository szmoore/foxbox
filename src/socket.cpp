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
	
Pipe Stdio(stdin, stdout);
	
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
		//Debug("Close socket with fd %d", m_sfd);
		if (fflush(m_file) != 0)
		{
			Fatal("Failed to fflush file descriptor %d - %s", m_sfd, StrError(errno));
		}
		if (close(m_sfd) != 0)
		{
			//TODO: There is a warning in the man page for close (2). Consider potential issues?
			Fatal("Failed to close file descriptor %d - %s", m_sfd, StrError(errno));
		}
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
Socket * Socket::Select(const vector<Socket*> & v, vector<Socket*> * readable, double timeout)
{
	
	
	int max_sfd = 0;
	fd_set readfds;
	FD_ZERO(&readfds);
	for (unsigned i = 0; i < v.size(); ++i)
	{
		if (!v[i]->Valid()) continue;
		if (v[i]->m_sfd > max_sfd) max_sfd = v[i]->m_sfd;
		FD_SET(v[i]->m_sfd, &readfds);
	}
	
	struct timeval tv;
	tv.tv_sec = (int)(timeout);
	tv.tv_usec = (timeout - (double)((int)timeout)) * 1000000;
	
	int err = 0;
	if (timeout >= 0)
		err = select(max_sfd+1, &readfds, NULL, NULL, &tv);
	else
		err = select(max_sfd+1, &readfds, NULL, NULL, NULL);
	
	if (err < 0)
	{
		if (errno != EINTR)
			Error("Error in select - %s", StrError(errno));
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
Socket * Socket::Select(size_t num_sockets, Socket * sockets)
{
	// Could probably do this accessing v.data() somehow ?
	vector<Socket*> v(num_sockets);
	for (unsigned i = 0; i < num_sockets; ++i)
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
	va_list ap2;
	va_start(ap, print);
	va_start(ap2, print);

	if (vfprintf(m_file, print, ap) < 0)
	{
		va_end(ap);
		
		Error("Error in vfprintf(3) - %s", StrError(errno));
		vfprintf(stderr, print, ap2);
		va_end(ap2);
		return false;
	}
	va_end(ap);
	return true;
}

bool Pipe::Send(const char * print, ...)
{
	if (!Valid()) //Is the process running...
		return false; 

	va_list ap;
	va_list ap2;
	va_start(ap, print);
	va_start(ap2, print);

	if (vfprintf(m_output.m_file, print, ap) < 0)
	{
		va_end(ap);
		
		Error("Error in vfprintf(3) - %s", StrError(errno));
		vfprintf(stderr, print, ap2);
		va_end(ap2);
		return false;
	}
	va_end(ap);
	return true;
}

int Socket::SendRaw(const void * buffer, size_t size)
{
	if (!Valid())
		return false;
	errno = 0;
	int written = write(m_sfd, buffer, size);
	if (written < 0)
	{
		Error("Wrote %d bytes, not %u - %s", written, size, StrError(errno));
		return written;
	}
	return written;
}

int Socket::GetRaw(void * buffer, size_t size)
{
	if (!Valid())
		return false;
	errno = 0;
	int received = read(m_sfd, buffer, size);
	if (received < 0 || errno != 0)
	{
		Error("Read %d bytes, not %u - %s", received, size, StrError(errno));
		return received;
	}
	// we are at the end of file
	// feof() on m_file will fail here, so close ourselves instead of relying on Valid :S
	// not supposed to mix stream and file descriptor functions...
	if (received == 0)
	{
		//Debug("Got 0 bytes from fd %d", m_sfd);
		Close();
	}
	return received;
}

bool Socket::CanSend(double timeout)
{
	if (!Valid())
	{
		//Error("Socket with fd %d is not valid.", m_sfd);
		return false;
	}
	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(m_sfd, &writefds);

	struct timeval tv;
	tv.tv_sec = (int)(timeout);
	tv.tv_usec = (timeout - (double)((int)timeout)) * 1000000;
	//Debug("Call select");
	int err = 0;
	if (timeout >= 0)
		err = select(m_sfd+1, NULL, &writefds, NULL, &tv);
	else
		err = select(m_sfd+1, NULL, &writefds, NULL, NULL);
	//Debug("End select");
	if (err < 0)
	{
		Error("Error in select - %s", StrError(errno));
		return false;
	}

	if (!FD_ISSET(m_sfd, &writefds))
	{
		//Warn("Timed out after %.2f seconds", timeout);
		return false; //Timed out
	}
	return true;
}

bool Socket::CanReceive(double timeout)
{
	if (!Valid())
	{
		//Error("Socket with fd %d is not valid.", m_sfd);
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
		Error("Error in select - %s", StrError(errno));
		return false;
	}

	if (!FD_ISSET(m_sfd, &readfds))
	{
		//Error("Timed out after %.2f seconds", timeout);
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
bool Socket::Get(string & buffer, size_t num_chars, double timeout)
{
	if (!Valid()) return false;
	if (!CanReceive(timeout)) return false;
	int c; size_t i = 0;
	for (c = fgetc(m_file); (i++ < num_chars && c != EOF); c = fgetc(m_file))
	{	
		buffer += c;
	}
	if (c == EOF)
	{
		Close();
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
		Close();
	}
	else if (inclusive)
	{
		buffer += c;
	}
	return (c != EOF);
}

int Socket::Dump(Socket & output, size_t block_size, double timeout)
{
	char * buffer = new char[block_size+1];
	int dumped = 0;

	while (CanReceive(timeout) && output.CanSend(timeout))
	{
		//Debug("Dumping from %d + %d bytes", dumped, block_size);
		int read = GetRaw(buffer, block_size);
		//Debug("Read from process");
		dumped += read;
		buffer[read+1] = '\0';
		//Debug("Writing output %s", buffer);
		if (read > 0)
			output.SendRaw(buffer, read);
	}
	delete [] buffer;
	return dumped;
}

pair<int, int> Socket::CatRaw(Socket & in1, Socket & out1, Socket & in2, Socket & out2, size_t block_size, double timeout)
{
	vector<Socket*> input(2);
	input[0] = &in1;
	input[1] = &in2;
	vector<Socket*> readable;
	
	char * buffer = new char[block_size];
	pair<int, int> bytes_sent;
	while ((in1.Valid() && out1.Valid()) || (in2.Valid() && out2.Valid()))
	{
		readable.clear();
		Socket::Select(input, &readable, timeout);
		for (auto it = readable.begin(); it != readable.end(); ++it)
		{
			Socket * in = *it;
			Socket * out = (in == &in1) ? &out1 : &out2;
			int * sent = (in == &in1) ? &(bytes_sent.first) : &(bytes_sent.second);
			//Debug("%s", (in == &in1) ? "ONE" : "TWO");
			string s("");
			int read = in->GetRaw(buffer, block_size);
			//Debug("Got s = %s", s.c_str());
			int written = out->SendRaw(buffer, block_size);
			if (read != written)
				Fatal("Discrepancy between result of GetRaw and SendRaw %d vs %d", read, written);
			*sent += read;

			
		}
	}
	delete [] buffer;
	return bytes_sent;
}

pair<int, int> Socket::Cat(Socket & in1, Socket & out1, Socket & in2, Socket & out2, const char * delims, double timeout)
{
	vector<Socket*> input(2);
	input[0] = &in1;
	input[1] = &in2;
	vector<Socket*> readable;
	pair<int, int> bytes_sent;
	while ((in1.Valid() && out1.Valid()) || (in2.Valid() && out2.Valid()))
	{
		readable.clear();
		Socket::Select(input, &readable, timeout);
		for (auto it = readable.begin(); it != readable.end(); ++it)
		{
			Socket * in = *it;
			Socket * out = (in == &in1) ? &out1 : &out2;
			int * sent = (in == &in1) ? &(bytes_sent.first) : &(bytes_sent.second);
			//Debug("%s", (in == &in1) ? "ONE" : "TWO");
			string s("");
			in->GetToken(s, delims,-1,true);
			//Debug("Got s = %s", s.c_str());
			out->Send(s);
			*sent += s.size();
		}
	}
	return bytes_sent;
}

// Helper only; read from both sockets and return the first location where they are not identical. Return -1 if they are identical. Store same and different portions.
int Socket::Compare(Socket & sock1, Socket & sock2, string * same, string * diff1, string * diff2, double timeout)
{
	int not_identical = -1;
	for (int byte = 0; sock1.CanReceive(timeout) || sock2.CanReceive(timeout); ++byte)
	{
		char c1 = '\0'; char c2 = '\0';
		if (sock1.CanReceive(timeout)) sock1.GetRaw(&c1, 1);
		if (sock2.CanReceive(timeout)) sock2.GetRaw(&c2, 1);
		if (not_identical < 0 && c1 != c2)
			not_identical = byte;
		if (not_identical < 0 && same != NULL)
			*same += c1;
		if (not_identical > 0)
		{
			if (diff1 != NULL)
				*diff1 += c1;
			if (diff2 != NULL)
				*diff2 += c2;
		}
	}
	return not_identical;
}

} //end namespace

