#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <unistd.h> //Needed to check permissions
#include "socket.h"
#include <vector>
#include <mutex>
#include <map>
#include <thread>
#include <dirent.h>

namespace Foxbox
{

/**
 * A wrapping class for an external program, which can exchange messages with the current process through stdin/stdout
 * Useful for attaching control of an operation to an external process - for example, AI for a game
 */
class Process : public Socket
{
	public:
		Process(const char * executeablePath, const std::map<std::string, std::string> & environment = {}, bool clear_environment=true); //Constructor
		virtual ~Process(); //Destructor

		
		bool Running() const;
		bool Paused() const;
		bool Pause();
		bool Continue();
		bool Wait();
		inline int Status() const {return m_status;}
		
	private:
		pid_t m_pid; //Process ID of the Process wrapped
		pid_t m_tid; //Thread ID of the thread in which the process has been wrapped
		
		
		bool m_paused;
		int m_status;
		
		class Manager
		{
			public:
				Manager();
				virtual ~Manager();
				
				std::map<pid_t, Process*> m_pid_map;
				bool m_started_sigchld_thread;
				std::thread m_sigchld_thread;
				std::mutex m_pid_mutex;
				sigset_t m_sigset;
				sigset_t m_sigusr1_set;
				bool m_running;
				
				static void SigchldThread(sigset_t * set, Manager * manager);
				static void Sigusr1Handler(int sig);
				void SpawnChild(Process * child);
				void DestroyChild(Process * child);
				static void GetThreads(std::vector<int> & tids);
		};
		
		static Manager s_manager;
};

}

#endif //PROCESS_H


