CPP=g++
MCPP=mpic++
LIBS= -lm -lboost_thread-mt -ltbb -lboost_program_options-mt -lgsl -lgslcblas
CFLAG = -g -O3 -Wall
DYNAMICSRC=dynamic.cpp getinput.cpp options.cpp scheduler.cpp worker.cpp
HEADERS=dynamic.hpp  getinput.hpp options.hpp scheduler.hpp worker.hpp

dynamic: $(DYNAMICSRC) $(HEADERS)
	$(MCPP) $(DYNAMICSRC) $(LIBS) -o dynamic $(CFLAG)
clean:
	rm -f dynamic
	rm -f *~	
	rm -rf \#*