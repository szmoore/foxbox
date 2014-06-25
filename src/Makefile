# Makefile for foxbox 

CXX = g++
FLAGS = --std=c++11 -D_POSIX_C_SOURCE=200112L -Wall -pedantic -g 
LIB = -lssl -lcrypto
PREPROCESSOR_FLAGS = 
OBJ = log.po socket.po tcp.po http.po
TARGET = libfoxbox.so

$(TARGET) : $(OBJ)
	$(CXX) -shared -Wl,-soname,$(TARGET) -o $(TARGET) $(OBJ) $(PREPROCESSOR_FLAGS) $(LIB)
	date > build_info
	uname -a >> build_info
%.po : %.cpp
	$(CXX) $(FLAGS) $(PREPROCESSOR_FLAGS) -fPIC -o $@ -c $<
 
clean :
	$(RM) $(TARGET) $(OBJ) build_info

clean_full: #cleans up all backup files
	$(RM) $(TARGET) $(OBJ) build_info
	$(RM) *.*~
	$(RM) *~
	$(RM) *.dat
	$(RM) *.out
	$(RM) *.err
