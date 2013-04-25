
CPP = g++
INCFLAGS = -Isrc
CPPFLAGS = -g -O2 -Wall -Wno-strict-aliasing
LIBS = -lboost_program_options -pthread -lzmq

CPPSRC = src/*.cpp
HPPSRC = src/*.hpp

server: $(CPPSRC) $(HPPSRC) test/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/server.cpp $(LIBS) -o bin/server


client: $(CPPSRC) $(HPPSRC) test/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/client.cpp $(LIBS) -o bin/client

