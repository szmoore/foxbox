#include <sstream>
#include <thread>
#include <algorithm>

#include <stdarg.h>

#include <cassert>

#include "process.h"
#include <vector>
#include <string.h>
#include <stdio.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>


using namespace std;
namespace Foxbox
{

map<pid_t, Process*> Process::s_all_pids;
mutex Process::s_pid_mutex;

/**
 * Constructor
 * @param executablePath - path to the program that will be run
 *
 * Creates two pipes - one for each direction between the parent process and the AI program
 * Forks the process. 
 *	The child process closes unused sides of the pipe, and then calls exec to replace itself with the AI program
 *	The parent process closes unused sides of the pipe, and sets up member variables - associates streams with the pipe fd's for convenience.
 */
Process::Process(const char * executablePath, const map<string, string> & environment, bool clear_environment) : Socket(), m_pid(0), m_paused(false)
{
	
		
	//See if file exists and is executable...
	if (access(executablePath, X_OK) != 0)
	{
		Fatal("File at path %s is not executable or does not exist", executablePath);
		m_pid = -1;
		return;
	}
	
	int sv[2];
	int error = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	if (error != 0)
	{
		//Should this be fatal?
		Fatal("Couldn't create socketpair -- %s", strerror(errno));
	}

	m_pid = fork();
	if (m_pid == 0)
	{
		// Setup environment variables
		if (clear_environment)
		{
			if (clearenv() != 0)
			{
				Fatal("Error clearing environment variables - %s", strerror(errno));
			}
		}
		for (map<string, string>::const_iterator it = environment.begin(); it != environment.end(); ++it)
		{
			// dop not use putenv, it is scary (uses non-const char*)
			if (setenv(it->first.c_str(), it->second.c_str(), true) != 0)
			{
				Fatal("Error setting environment variable %s=%s - %s", it->first.c_str(), it->second.c_str(), strerror(errno));
			}
		}
			
		//TODO: Fix possible bug here if the parent process was a daemon
		dup2(sv[0],fileno(stdin)); 
		dup2(sv[0],fileno(stdout));

		if (access(executablePath, X_OK) == 0) //Check we STILL have permissions to start the file
		{
			execl(executablePath, executablePath, (char*)(NULL)); ///Replace process with desired executable
			//execv(executablePath,arguments); ///Replace process with desired executable
		}
		Fatal("Error in execl trying to start program %s - %s", executablePath, strerror(errno));
		// How to deal with this in the parent?
	}
	else
	{
		Process::s_pid_mutex.lock();
		
		if (Process::s_all_pids.size() == 0)
		{
			signal(SIGCHLD, Process::sigchld_handler);
		}
		
		Process::s_all_pids[m_pid] = this;
		
		
		Process::s_pid_mutex.unlock();
		
		Socket::m_sfd = sv[1];
		Socket::m_file = fdopen(Socket::m_sfd,"r+");
		setbuf(Socket::m_file, NULL);
	}
		
}

/**
 * Destructor
 * Writes EOF to the wrapped program and then closes all streams
 * Kills the wrapped program if it does not exit within 1 second.
 */
Process::~Process()
{
	Debug("Destructor for Process with m_pid = %d", m_pid);
	Close(); // flush files
	
	// remove from list of processes
	Process::s_pid_mutex.lock();
	map<pid_t, Process*>::iterator it = Process::s_all_pids.find(m_pid);
	if (it != Process::s_all_pids.end())
		Process::s_all_pids.erase(it);
	Process::s_pid_mutex.unlock();	
	
	if (Running()) //Check if the process created is still running...
	{
		Debug("Kill process %d", m_pid);
		kill(m_pid, SIGQUIT); //kill it
	}
	
	

}

/**
 * Forces the program to pause by sending SIGSTOP
 * Process can be resumed by calling Continue() (which sends SIGCONT)
 * @returns true if the program could be paused, false if it couldn't (probably because it wasn't running)
 */
bool Process::Pause()
{
	if (m_pid > 0 && kill(m_pid,SIGSTOP) == 0)
	{
		m_paused = true;
		return true;
	}
	return false;
}

/**
 * Causes a paused program to continue
 * @returns true if the program could be continued, false if it couldn't (probably because it wasn't running)
 */
bool Process::Continue()
{
	if (m_pid > 0 && kill(m_pid,SIGCONT) == 0)
	{
		m_paused = false;
		return true;
	}
	return false;
}

/**
 * @returns true iff the program is paused
 */
bool Process::Paused() const
{
	return m_paused;
}

/**
 * Returns true iff the process is running
 * @returns that
 */
bool Process::Running() const
{
	return (m_pid > 0 && kill(m_pid,0) == 0);
}

void Process::sigchld_handler(int signum)
{
	Debug("Got signal %d", signum);
	//return;
	Process::s_pid_mutex.lock();
	int status;
	while (true)
	{
		pid_t pid = waitpid(-1, &status, WNOHANG);
		Debug("Signal %d for process %d", signum, pid);
		if (pid <= 0) break;
		map<pid_t, Process*>::iterator it = Process::s_all_pids.find(pid);
		if (it == Process::s_all_pids.end())
			break;
		it->second->Close();
		Process::s_all_pids.erase(it);
		Debug("Handled signal");
	}
	Process::s_pid_mutex.unlock();
	Debug("Finished handling signals");
	
}


}

