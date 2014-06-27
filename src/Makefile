# Makefile for foxbox 

CXX = g++
FLAGS = --std=c++11 -D_POSIX_C_SOURCE=200112L -Wall -pedantic -g 
LIB = 
PREPROCESSOR_FLAGS = 
POBJ = base64.po sha1.po log.po socket.po tcp.po http.po websocket.po
DYNAMIC = ../libfoxbox.so
OBJ = base64.o sha1.o log.o socket.o tcp.o http.o websocket.o
STATIC = ../libfoxbox.a

all : $(DYNAMIC) $(STATIC)

$(DYNAMIC) : $(POBJ)
	$(CXX) -shared -Wl,-soname,$(DYNAMIC) -o $(DYNAMIC) $(POBJ) $(PREPROCESSOR_FLAGS) $(LIB)
	date > ../build_info
	uname -a >> ../build_info
	
$(STATIC) : $(OBJ)
	ar rcs $(STATIC) $(OBJ)

%.o : %.cpp
	$(CXX) $(FLAGS) $(PREPROCESSOR_FLAGS) -o $@ -c $<

%.po : %.cpp
	$(CXX) $(FLAGS) $(PREPROCESSOR_FLAGS) -fPIC -o $@ -c $<
	


 
clean :
	$(RM) $(DYNAMIC) $(POBJ) ../build_info $(STATIC) $(OBJ)

clean_full: #cleans up all backup files $(STATIC) $(OBJ)
	$(RM) $(DYNAMIC) $(POBJ) ../build_info
	$(RM) *.*~
	$(RM) *~
	$(RM) *.dat
	$(RM) *.out
	$(RM) *.err
