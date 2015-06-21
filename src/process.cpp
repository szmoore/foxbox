#include <sstream>
#include <thread>
#include <algorithm>

#include <stdarg.h>

#include <cassert>

#include "process.h"
#include <vector>
#include <thread>
#include <string.h>
#include <stdio.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>



using namespace std;
namespace Foxbox
{

Process::Manager Process::s_manager;

/**
 * Constructor
 * TODO: Update documentation (uses unix domain sockets, not pipes)
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

		if (access(executablePath, X_OK) == 0) //Check we STILL have permissions to start the file //REDUNDANT
		{
			execl(executablePath, executablePath, (char*)(NULL)); ///Replace process with desired executable
			//execv(executablePath,arguments); ///Replace process with desired executable
		}
		Fatal("Error in execl trying to start program %s - %s", executablePath, strerror(errno));
		// How to deal with this in the parent?
	}
	else
	{
		Socket::m_sfd = sv[1];
		Socket::m_file = fdopen(Socket::m_sfd,"r+");
		setbuf(Socket::m_file, NULL);
		m_tid = syscall(SYS_gettid);
		Debug("Process is valid in thread %d", m_tid);
		s_manager.SpawnChild(this);
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
	Process::s_manager.DestroyChild(this);

	
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

Process::Manager::Manager() : m_pid_map(), m_started_sigchld_thread(false), m_sigchld_thread(), m_pid_mutex(), m_running(true)
{
	// all threads ignore SIGCHLD
	sigemptyset(&m_sigset);
	sigaddset(&m_sigset, SIGCHLD);
	sigaddset(&m_sigset, SIGPIPE); // also ignore SIGPIPE
	sigprocmask(SIG_BLOCK, &m_sigset, NULL);
	Debug("Constructed Process manager, block SIGCHLD and SIGPIPE");
	
	struct sigaction sa;
	sa.sa_handler = Process::Manager::Sigusr1Handler;
	sa.sa_flags = 0;
	
	if (sigaction(SIGUSR1, &sa, NULL) != 0)
		Fatal("Could not setup signal handler for SIGUSR1 - %s", strerror(errno));
}

Process::Manager::~Manager()
{
	if (m_started_sigchld_thread)
	{
		m_running = false;
		Debug("Kill child.");
		kill(0, SIGCHLD);
		m_sigchld_thread.join();
		Debug("Child has joined");
	}
	sigprocmask(SIG_UNBLOCK, &m_sigset, NULL);
}

void Process::Manager::SpawnChild(Process * child)
{
	sigprocmask(SIG_BLOCK, &m_sigset, NULL);
	
	Debug("Spawning child process; add to m_pid_map");
	m_pid_mutex.lock();
	m_pid_map[child->m_pid] = child;
	if (!m_started_sigchld_thread)
	{
		m_started_sigchld_thread = true;
		Debug("Starting sigchld thread");
		m_sigchld_thread = thread(Process::Manager::SigchldThread, &m_sigset, this);
	}
	m_pid_mutex.unlock();
}

void Process::Manager::DestroyChild(Process * child)
{
	Debug("Destroying child process; remove from m_pid_map");
	m_pid_mutex.lock();
	map<pid_t, Process*>::iterator it = m_pid_map.find(child->m_pid);
	if (it != m_pid_map.end())
		m_pid_map.erase(it);
	m_pid_mutex.unlock();	
	
}

void Process::Manager::Sigusr1Handler(int sig)
{
	Debug("SIGUSR1 interrupt %d here caught by thread %d", sig, syscall(SYS_gettid));
}

void Process::Manager::GetThreads(vector<int> & tids)
{
	tids.clear();
	DIR * proc_dir;
	stringstream dirname;
	dirname << "/proc/" << getpid() << "/task";
	proc_dir = opendir(dirname.str().c_str());
	if (!proc_dir)
		Fatal("%s could not be opened - %s", dirname.str().c_str(), strerror(errno));
	struct dirent * entry;
	while ((entry = readdir(proc_dir)) != NULL)
	{
		char * end;
		long int tid = strtol(entry->d_name, &end, 10);
		if (*end == '\0')
		{
			tids.push_back(tid);
		}
	}
}

void Process::Manager::SigchldThread(sigset_t * set, Process::Manager * master)
{

	Debug("Sigchld thread starts");
	//pid_t this_tid = syscall(SYS_gettid);
	while (master->m_running)
	{
		int sig = 0;
		if (sigwait(set, &sig) != 0 && sig != SIGCHLD)
		{
			Error("Got wrong signal (%d not %d) or error in sigwait - %s", sig, SIGCHLD, strerror(errno));
			break;
		}
		Debug("Got SIGCHLD %d", sig);
		master->m_pid_mutex.lock();
		
		while (true)
		{
			int status = 0;
			pid_t pid = waitpid(-1, &status, WNOHANG);
			Debug("PID of child was %d", pid);
			if (pid <= 0) break;
			map<pid_t, Process*>::iterator it = master->m_pid_map.find(pid);
			if (it == master->m_pid_map.end()) continue;
			it->second->Close();
			syscall(SYS_tgkill, getpid(), it->second->m_tid, SIGUSR1);
			master->m_pid_map.erase(it);
		}
		
		// we need to interrupt any system calls involved with the child process
		/*vector<int> tids;
		Process::Manager::GetThreads(tids);
		for (unsigned i = 0; i < tids.size(); ++i)
		{
			if (tids[i] == this_tid) continue;
			syscall(SYS_tgkill, getpid(), tids[i], SIGUSR1);
		}
		*/
		master->m_pid_mutex.unlock();
	}
	Debug("Sigchld thread exits here");
}

}

