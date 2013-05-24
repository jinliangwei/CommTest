
CPP = g++
INCFLAGS = -Isrc
CPPFLAGS = -g -O2 -Wall -Wno-strict-aliasing
LIBS = -lboost_program_options -pthread -lzmq

CPPSRC = src/*.cpp
HPPSRC = src/*.hpp

basic: server client
chain: chain-server chain-client chain-serclient
benchmark:benserver benclient


server: $(CPPSRC) $(HPPSRC) test/basic/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/basic/server.cpp $(LIBS) -o bin/server


client: $(CPPSRC) $(HPPSRC) test/basic/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/basic/client.cpp $(LIBS) -o bin/client

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

benserver: $(CPPSRC) $(HPPSRC) test/benchmark/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/benchmark/server.cpp $(LIBS) -o bin/benserver


benclient: $(CPPSRC) $(HPPSRC) test/benchmark/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/benchmark/client.cpp $(LIBS) -o bin/benclient

clean:
	rm -rf bin