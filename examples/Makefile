# Makefile for foxbox examples

CXX = g++
FLAGS = --std=c++11 -D_POSIX_C_SOURCE=200112L -Wall -pedantic -g -I../src -pthread
LIB = -L../src -lfoxbox -lssl -lcrypto
PREPROCESSOR_FLAGS = 
ALL = netcat proxy threadedserver threadedclient httpserver httpproxy wget wsserver wscat 

all : $(ALL)


% : %.cpp ../src/libfoxbox.so
	$(CXX) $(FLAGS) $(PREPROCESSOR_FLAGS) -o $@ $< $(LIB)

clean :
	rm -f $(ALL)
