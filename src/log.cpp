/**
 * @file log.cpp
 * @brief Functions for log messages, debugging and errors - Definitions
 * @see log.h - Declarations
 */


#include <unistd.h>
#include <stdarg.h>

#include "log.h"

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

	// End log messages with a newline
	fprintf(stderr, "\n");
}

/**
 * Handle a Fatal error in the program by printing a message and exiting the program
 *	CALLING THIS FUNCTION WILL CAUSE THE PROGAM TO EXIT
 * @param funct - Name of the calling function
 * @param file - Source file in which Fatal was called
 * @param line - Line at which Fatal
 * @param fmt - A format string
 * @param ... - Arguments to be printed according to the format string
 */
void FatalEx(const char * funct, const char * file, int line, ...)
{
	const char *fmt;
	va_list va;
	va_start(va, line);
	fmt = va_arg(va, const char*);
	
	if (fmt == NULL)
	{
		// Fatal error in the Fatal function.
		// (This really shouldn't happen unless someone does something insanely stupid)
		Fatal("Format string is NULL");
		return; // Should never get here
	}

	if (funct == NULL)
		funct = unspecified_funct;

	fprintf(stderr, "%s (%s:%d) : FATAL - ", funct, file, line);

	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}


}
