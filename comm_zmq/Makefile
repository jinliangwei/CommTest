
CPP = g++
INCFLAGS = -Isrc
CPPFLAGS = -g -O2 -Wall -Wno-strict-aliasing
LIBS = -lboost_program_options -lboost_thread-mt -pthread -lzmq

CPPSRC = src/*.cpp
HPPSRC = src/*.hpp

basic: server client
chain: chain-server chain-client chain-serclient
benchmark:benserver benclient
multithread: muserver mclient
async: aserver aclient

server: $(CPPSRC) $(HPPSRC) test/basic/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/basic/server.cpp $(LIBS) -o bin/server

client: $(CPPSRC) $(HPPSRC) test/basic/client.cpp
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


mserver: $(CPPSRC) $(HPPSRC) test/multi_thread/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/multi_thread/server.cpp $(LIBS) -o bin/mserver


mclient: $(CPPSRC) $(HPPSRC) test/multi_thread/client.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/multi_thread/client.cpp $(LIBS) -o bin/mclient

aserver: $(CPPSRC) $(HPPSRC) test/async/server.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/async/server.cpp $(LIBS) -o bin/aserver

aclient: $(CPPSRC) $(HPPSRC) test/async/client.cpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/async/client.cpp $(LIBS) -o bin/aclient

clean:
	rm -rf bin