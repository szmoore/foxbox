/**
 * @file debugutils.h
 * @brief Helper functions for debugging multithreaded programs
 * @see threadutls.cpp - Definitions
 */
#ifndef _DEBUGUTILS_H
#define _DEBUGUTILS_H

#include <thread>
#include <mutex>
#include <map>
#include <unistd.h>
#include <sys/syscall.h>
#include <set>

namespace Foxbox
{
namespace DebugUtils
{
	
inline int ThreadID() {return syscall(SYS_gettid);}

/** Class to be used as a static member variable in functions, to identify possible critical sections **/
class CriticalSection
{
	public:
		CriticalSection() : m_section_mutex(), m_member_mutex(), m_active_threads() {}
		virtual ~CriticalSection() {}
		
		int Enter(bool lock);
		int Leave(bool lock);
		inline int ThreadCount() 
		{
			m_member_mutex.lock(); 
			int result = m_active_threads.size(); 
			m_member_mutex.unlock(); 
			return result;
		}
		
	private:
		std::mutex m_section_mutex;
		std::mutex m_member_mutex;
		std::set<int> m_active_threads;
};

#define VERIFY_CRITICAL \
	static DebugUtils::CriticalSection crit; \
	if (crit.Enter(false) > 1) \
		Warn("%d threads in critical section", crit.ThreadCount());
	
#define END_VERIFY_CRITICAL \
	crit.Leave(false);


}
	
}

#endif //_DEBUGUTILS_H

