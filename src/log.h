/**
 * @file log.h
 * @brief Declaration of functions for printing general log messages and/or terminating program after a fatal error
 */

#ifndef _LOG_H
#define _LOG_H

#include <cstdio>
#include <cstdlib>
#include <string>

namespace Foxbox
{

inline std::string MethodName(const std::string& pretty_function)
{
    size_t colons = pretty_function.find("::");
    size_t begin = pretty_function.substr(0,colons).rfind(" ") + 1;
    size_t end = pretty_function.rfind("(") - begin;
    return pretty_function.substr(begin,end) + "()"; // TODO: Remove memory leak caused by this line
}

#define __METHOD_NAME__ MethodName(__PRETTY_FUNCTION__).c_str()

//To get around a 'pedantic' C99 rule that you must have at least 1 variadic arg, combine fmt into that.
#define Log(level, ...) LogEx(level, __METHOD_NAME__, __FILE__, __LINE__, __VA_ARGS__)
#define Fatal(...) FatalEx(__METHOD_NAME__, __FILE__, __LINE__, __VA_ARGS__)

// An enum to make the severity of log messages human readable in code
enum {LOGERR=0, LOGWARN=1, LOGNOTE=2, LOGINFO=3,LOGDEBUG=4};

#define LOGVERBOSITY 5 // Anything with a log level below this will get printed

#define Debug(...) Log(LOGDEBUG, __VA_ARGS__)
#define Error(...) Log(LOGERR, __VA_ARGS__)
#define Warn(...) Log(LOGWARN, __VA_ARGS__)

extern void LogEx(int level, const char * funct, const char * file, int line, ...); // General function for printing log messages to stderr
extern void FatalEx(const char * funct, const char * file, int line, ...); // Function that deals with a fatal error (prints a message, then exits the program).

}

#endif //_LOG_H

//EOF
