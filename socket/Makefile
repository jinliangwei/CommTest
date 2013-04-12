
CPP = g++
INCFLAGS = -I.
CPPFLAGS = -g -O2 -Wall -Wno-strict-aliasing
LIBS = -lboost_program_options -pthread

CPPSRC = src/tcp_comm.cpp src/util.cpp
HPPSRC = src/tcp_comm.hpp src/util.hpp

sch-worker: scheduler worker

scheduler: src/scheduler.cpp $(CPPSRC) $(HPPSRC) src/threadpool.cpp src/threadpool.hpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) src/scheduler.cpp src/threadpool.cpp $(LIBS) -o bin/scheduler

worker: src/worker.cpp $(CPPSRC) $(HPPSRC) src/threadpool.cpp src/threadpool.hpp
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) src/worker.cpp src/threadpool.cpp $(LIBS) -o bin/worker

testserver: test/server.cpp $(CPPSRC) $(HPPSRC)
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/server.cpp $(LIBS) -o bin/testserver

testclient: test/client.cpp $(CPPSRC) $(HPPSRC)
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/client.cpp $(LIBS) -o bin/testclient

testmsgq: test/msgqtest.cpp $(CPPSRC) $(HPPSRC)
	mkdir -p bin
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $(CPPSRC) test/msgqtest.cpp $(LIBS) -o bin/testmsgq

clean:
	rm -rf bin