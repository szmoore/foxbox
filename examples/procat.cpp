/**
 * @file procat.cpp
 * @brief Runs a process and cats stdin/stdout
 */

#include "foxbox.h"
#include <cassert>

using namespace std;
using namespace Foxbox;

int main(int argc, char ** argv)
{
	Process proc(argv[1]);
	if (!proc.Running())
	{
		Fatal("Could not run process %s", argv[1]);
	}
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	Socket input(stdin);
	Socket output(stdout);
	
	Socket::Cat(input, proc, proc, output);
	
}
