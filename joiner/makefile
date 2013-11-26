    
libbase=/usr/local/lib
incbase=/usr/local/include

OPTFLAGS	= -O2
INCFLAGS	= -I $(incbase)
LIBS		= -lgeos -lspatialindex -L $(libbase)

CC = gcc
CXX = g++


all: resque

resque: resque.cpp
	$(CXX) resque.cpp $(INCFLAGS) $(LIBS) $(OPTFLAGS) -o $@
clean:
	@rm -f resque

