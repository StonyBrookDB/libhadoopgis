
spatial=/home/aaji/softs

UNAME		= $(shell uname)
OPTFLAGS	= -O2 -Wno-write-strings
INCBASE		= -I. -I/usr/local/include
LIBBASE		= -lspatialindex -lgeos -L/usr/local/lib

ifneq (,$(findstring Linux,$(shell uname -a)))
    INCFLAGS = -I$(spatial)/include
    LIBS = -L$(spatial)/lib
else 
    ifeq ($(UNAME), Darwin)
    INCFLAGS = -I /Library/Frameworks/GEOS.framework/Headers
    LIBS = -L /Library/Frameworks/GEOS.framework/unix/lib
endif
endif

CFLAGS      = $(OPTFLAGS) $(INCBASE) $(INCFLAGS) 
LDFLAGS     = $(LIBBASE) $(LIBS)

ifeq ($(UNAME), Darwin)
  CC = g++ -arch x86_64
else 
  CC = g++
endif


all: hgtiler

debug: CXX += -DDEBUG -g
debug: CC += -DDEBUG -g
debug: hgtiler

# gengetopt -i options.ggo -F cmdline

cmd.o: options.ggo cmdline.h cmdline.c
	$(CC) -c cmdline.c -o cmd.o

hgtiler: cmd.o tiler.cpp hadoopgis.h tokenizer.h
	$(CC) tiler.cpp cmd.o $(CFLAGS) $(LDFLAGS) $(OPTFLAGS) -o hgtiler
clean:
	@rm -f *.o hgtiler

