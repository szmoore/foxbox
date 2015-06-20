#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <unistd.h> //Needed to check permissions
#include "socket.h"
#include <vector>
#include <mutex>
#include <map>
#include <thread>

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

		virtual bool Valid() {return Running() && Socket::Valid();}
		bool Running() const;
		bool Paused() const;
		bool Pause();
		bool Continue();
		
		static void HandleFlags(); // This should be private, but Foxbox namespace needs it. It shouldn't do anything bad if a user calls it.

	private:
		pid_t m_pid; //Process ID of the Process wrapped
		
		
		bool m_paused;
		
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
				
				static void SigchldThread(sigset_t * set, Manager * manager);
				void SpawnChild(Process * child);
				void DestroyChild(Process * child);
		};
		
		static Manager s_manager;
};

}

#endif //PROCESS_H


