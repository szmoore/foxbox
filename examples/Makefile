# Makefile for foxbox examples

CXX = g++
FLAGS = --std=c++11 -D_POSIX_C_SOURCE=200112L -Wall -pedantic -g -I../src -pthread
# Change option before foxbox for dynamic/static
LIB = -L.. -Wl,-Bstatic -lfoxbox -Wl,-Bdynamic
PREPROCESSOR_FLAGS = 
ALL = netcat proxy threadedserver threadedclient httpserver httpproxy wget wsserver wscat procat

all : $(ALL)


% : %.cpp ../libfoxbox.a
	$(CXX) $(FLAGS) $(PREPROCESSOR_FLAGS) -o $@ $< $(LIB)

clean :
	rm -f $(ALL)
