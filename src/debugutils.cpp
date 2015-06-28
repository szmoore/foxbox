
#include "debugutils.h"

using namespace std;
namespace Foxbox
{
namespace DebugUtils
{

int CriticalSection::Enter(bool lock)
{
	m_member_mutex.lock();
	m_active_threads.insert(ThreadID());
	
	
	int result = m_active_threads.size();
	m_member_mutex.unlock();
	
	if (lock) m_section_mutex.lock();
	return result;
}

int CriticalSection::Leave(bool unlock)
{
	m_member_mutex.lock();
	m_active_threads.erase(ThreadID());
	
	int result = m_active_threads.size();
	m_member_mutex.unlock();
	
	if (unlock) m_section_mutex.unlock();
	return result;
}

}
}
