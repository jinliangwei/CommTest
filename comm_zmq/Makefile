
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

chain-server: $(CPPSRC) $(HPPSRC) test/chain/server.cpp
	mkdir -p bin
	mkdir -p bin/chain
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/chain/server.cpp $(LIBS) -o bin/chain/server

chain-client: $(CPPSRC) $(HPPSRC) test/chain/client.cpp
	mkdir -p bin
	mkdir -p bin/chain
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/chain/client.cpp $(LIBS) -o bin/chain/client

chain-serclient: $(CPPSRC) $(HPPSRC) test/chain/server_client.cpp
	mkdir -p bin
	mkdir -p bin/chain
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/chain/server_client.cpp $(LIBS) -o bin/chain/serclient


clean:
	rm -rf bin