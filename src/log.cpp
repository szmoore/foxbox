/**
 * @file log.cpp
 * @brief Functions for log messages, debugging and errors - Definitions
 * @see log.h - Declarations
 */

#include <unistd.h>
#include <stdarg.h>
#include <exception>
#include "log.h"
#include <mutex> // if we care about thread safety...
#include <execinfo.h> // For backtrace
#include <sys/syscall.h> // for gettid
#include <cstring>
#include <map>

using namespace std;

namespace Foxbox
{

/** Used when the function name can't be determined **/
static const char * unspecified_funct = "???";

/**
 * Print a message to stderr
 * @param level - Specify how severe the message is.
	If level is higher (less urgent) than LOGVERBOSITY no message will be printed
 * @param funct - Should indicate the calling function's name
	If this is NULL, will print unspecified_funct
 * @param file - Source file in which Log was called
 * @param line - Line number at which Log was called
 * @param fmt - A format string
 * @param ... - Arguments to be printed according to the format string
 */
void LogEx(int level, const char * funct, const char * file, int line ...)
{
	static mutex s_mutex;
	s_mutex.lock();
	
	const char *fmt;
	va_list va;
	va_start(va, line);
	fmt = va_arg(va, const char*);
	
	if (fmt == NULL) // sanity check
		Fatal("Format string is NULL");

	// Don't print the message unless we need to
	if (level > LOGVERBOSITY)
		return;

	if (funct == NULL)
		funct = unspecified_funct;

	// Make a human readable severity string
	const char *severity;
	switch (level)
	{
		case LOGERR:
			severity = "ERROR";
			break;
		case LOGWARN:
			severity = "WARNING";
			break;
		case LOGNOTE:
			severity = "NOTICE";
			break;
		case LOGINFO:
			severity = "INFO";
			break;
		default:
			severity = "DEBUG";
			break;
	}

	// Print: Severity string, function name first
	fprintf(stderr, "%s : %s (%s:%d) - ", severity, funct, file, line);

	// Then pass additional arguments with the format string to vfprintf for printing
	vfprintf(stderr, fmt, va);
	va_end(va);
	
	// Print function that called the function that called LogEx
	
	#ifdef LOG_BACKTRACE
	void * trace[3];
	int size = backtrace(trace, 3);
	char ** trace_names = backtrace_symbols(trace, size);
	fprintf(stderr, " <-[");
	for (int i = 2; i < size; ++i)
	{
		fprintf(stderr, "%s", trace_names[i]);
		if (i < size-1)
			fprintf(stderr, " <- ");
	}
	fprintf(stderr, " ]");
	#endif //LOG_BACKTRACE
	
	
	#ifdef LOG_PIDANDTID
	// Print PID and TID
	long int tid = syscall(SYS_gettid);
	int pid = getpid();
	fprintf(stderr, " [%d Thread %ld]", pid, tid-pid); 
	#endif //LOG_PIDANDTID
	// End log messages with a newline
	fprintf(stderr, "\n");
	s_mutex.unlock();
}

// Because strerror may not be thread safe. Blergh.
// This is disgusting, but since it is only called on errors, don't care too much about performance.
// So it can just be substituted exactly for strerror except TitleCase.
// Could develop a singleton class to make the errors user accessible for debugging, but probably no point
const char * StrError(int errnum)
{
	static mutex s_mutex;
	static map<int, string> s_thread_map;
	const char * result = '\0';
	s_mutex.lock();
	string & s = s_thread_map[syscall(SYS_gettid)];
	s = strerror(errnum);
	result = s.c_str();
	s_mutex.unlock();
	return result;
}

}
