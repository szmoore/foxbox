# Makefile for foxbox examples

CXX = g++
FLAGS = --std=c++14 -D_POSIX_C_SOURCE=200112L -Wall -pedantic -g -I../src -pthread
# Change option before foxbox for dynamic/static
LIB = -L.. -Wl,-Bstatic -lfoxbox -Wl,-Bdynamic -rdynamic
PREPROCESSOR_FLAGS = 
#ALL = httpserver cgistresstest
ALL = netcat proxy threadedserver threadedclient httpserver httpproxy wget wsserver wscat procat 3des 3des-netcat

all : $(ALL)


% : %.cpp ../libfoxbox.a
	$(CXX) $(FLAGS) $(PREPROCESSOR_FLAGS) -o $@ $< $(LIB)

clean :
	rm -f $(ALL)
