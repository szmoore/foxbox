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
Process::Process(const char * executablePath) : Socket(), m_pid(0), m_paused(false)
{
	
		
	//See if file exists and is executable...
	if (access(executablePath, X_OK) != 0)
	{
		m_pid = -1;
		return;
	}
	
	int sv[2];
	int error = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	if (error != 0)
	{
		Fatal("Couldn't create socketpair -- %s", strerror(errno));
	}

	m_pid = fork();
	if (m_pid == 0)
	{
		
		//TODO: Fix possible bug here if the process is already a daemon
		dup2(sv[0],fileno(stdin)); 
		dup2(sv[0],fileno(stdout));

		//TODO: Somehow force the exec'd process to be unbuffered
		setbuf(stdin, NULL); //WARNING: These lines don't appear to have any affect
		setbuf(stdout, NULL); //You should add them at the start of the wrapped program.
					//If your wrapped program is not written in C/C++, you will probably have a problem
				

		if (access(executablePath, X_OK) == 0) //Check we STILL have permissions to start the file
		{
			execl(executablePath, executablePath, (char*)(NULL)); ///Replace process with desired executable
			//execv(executablePath,arguments); ///Replace process with desired executable
		}
		perror("execv error:\n");
		fprintf(stderr, "Process::Process - Could not run program \"%s\"!\n", executablePath);
		exit(EXIT_FAILURE); //We will probably have to terminate the whole program if this happens
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
	if (Running()) //Check if the process created is still running...
	{
		kill(m_pid, SIGKILL); //kill it
	}
	
	// remove this pid
	Process::s_pid_mutex.lock();
	map<pid_t, Process*>::iterator it = Process::s_all_pids.find(m_pid);
	if (it != Process::s_all_pids.end())
	{
		Process::s_all_pids.erase(it);
	}
	Process::s_pid_mutex.unlock();
		
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
	Process::s_pid_mutex.lock();
	
	int status;
	while (true)
	{
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid <= 0) break;
		
		Process * proc = Process::s_all_pids[pid];
		assert(proc != NULL);
		proc->Close();
		proc->m_pid = -1;
	}
	
	
	Process::s_pid_mutex.unlock();
	
	
}


}

