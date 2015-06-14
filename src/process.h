#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <unistd.h> //Needed to check permissions
#include "socket.h"

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

	
		bool Running() const;
		bool Paused() const;
		bool Pause();
		bool Continue();

	private:
		pid_t m_pid; //Process ID of the Process wrapped
		bool m_paused;
	
};

}

#endif //PROCESS_H


