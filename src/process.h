#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <unistd.h> //Needed to check permissions
#include "socket.h"
#include <vector>
#include <mutex>
#include <map>

namespace Foxbox
{

/**
 * A wrapping class for an external program, which can exchange messages with the current process through stdin/stdout
 * Useful for attaching control of an operation to an external process - for example, AI for a game
 */
class Process : public Socket
{
	public:
		Process(const char * executeablePath); //Constructor
		virtual ~Process(); //Destructor

		virtual bool Valid() {return Running() && Socket::Valid();}
		bool Running() const;
		bool Paused() const;
		bool Pause();
		bool Continue();

	private:
		pid_t m_pid; //Process ID of the Process wrapped
		static std::map<pid_t, Process*> s_all_pids;
		static std::mutex s_pid_mutex;
		
		static void sigchld_handler(int sig);
		
		bool m_paused;
	
};

}

#endif //PROCESS_H


