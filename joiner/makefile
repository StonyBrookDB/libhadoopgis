
prefix=/usr/local

libbase=/home/aaji/softs/lib
incbase =/home/aaji/softs/include

OPTFLAGS	= -O2
INCFLAGS	= -I $(incbase)
LIBS		= -lgeos -lspatialindex -L $(libbase)

CC = gcc
CXX = g++


all: resque

resque: resque.cpp
	$(CXX) resque.cpp $(INCFLAGS) $(LIBS) $(OPTFLAGS) -o $@
install:
	mkdir -p $(prefix)/bin
	cp resque $(prefix)/bin

clean:
	@rm -f resque

