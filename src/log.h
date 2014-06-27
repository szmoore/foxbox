/**
 * @file log.h
 * @brief Functions for log messages, debugging and errors - Declarations
 * @see log.cpp - Definitions
 */

#ifndef _LOG_H
#define _LOG_H

#include <cstdio>
#include <cstdlib>
#include <string>

namespace Foxbox
{
	
/** General function for printing log messages to stderr  - DO NOT USE **/
extern void LogEx(int level, const char * funct, const char * file, int line, ...); 
/** Function that deals with a fatal error (prints a message, then exits the program) - DO NOT USE**/
extern void FatalEx(const char * funct, const char * file, int line, ...); 
// To get around a 'pedantic' C99 rule that you must have at least 1 variadic arg, 
// combine fmt into that.


/** Get a "Human Readable" version of a C++ function name **/
inline std::string MethodName(const std::string& pretty_function)
{
    size_t colons = pretty_function.find("::");
    size_t begin = pretty_function.substr(0,colons).rfind(" ") + 1;
    size_t end = pretty_function.rfind("(") - begin;
    // TODO: Remove memory leak caused by this line 
    // :(
    return pretty_function.substr(begin,end) + "()"; 
}

#define __METHOD_NAME__ MethodName(__PRETTY_FUNCTION__).c_str()

/** Log at a level of severity **/
#define Log(level, ...) LogEx(level, __METHOD_NAME__, __FILE__, __LINE__, __VA_ARGS__)
/** Fatal error - TERMINATES THE PROCESS **/
#define Fatal(...) FatalEx(__METHOD_NAME__, __FILE__, __LINE__, __VA_ARGS__)

/** An enum to make the severity of log messages human readable in code **/
enum {LOGERR=0, LOGWARN=1, LOGNOTE=2, LOGINFO=3,LOGDEBUG=4};

/** Anything with log level above this will not be printed **/
#define LOGVERBOSITY 5

/** Commonly used Log levels **/
#define Debug(...) Log(LOGDEBUG, __VA_ARGS__)
//#define Debug(...) // Use to remove all debug messages
#define Error(...) Log(LOGERR, __VA_ARGS__)
#define Warn(...) Log(LOGWARN, __VA_ARGS__)





}

#endif //_LOG_H

//EOF
