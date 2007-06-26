CC      = gcc
CXX		= g++
LD		= g++

TARGET = ecalc
FLUID = fluid
PREFIX  = 

IFLAGS = -I/usr/X11/include
LFLAGS = -L/usr/X11R6/lib -L/usr/local/lib
CPPFLAGS  = -O2 $(IFLAGS) $(DFLAGS) -Wall 
CXXFLAGS  = -O2 $(IFLAGS) $(DFLAGS) -Wall 
CFLAGS  = -O2 $(IFLAGS) $(DFLAGS) -Wall 
LLIBS =
LIBS = -lfltk -lX11 -lm
OBJS =	Main.o SciCalc.o


all : $(TARGET)

.SUFFIXES:	.cpp .hh .o .fld

.fld.cpp:
	$(FLUID) -c $<
.cpp.o :
	$(CXX) -c $< -o $(<:.cpp=.o) $(CXXFLAGS)
.c.o :
	$(CC) -c $< -o $(<:.c=.o) $(CPPFLAGS)

$(TARGET) : $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(LFLAGS) $(LLIBS) $(LIBS)
	strip $(TARGET)

clean:
	-rm *.o
	-rm $(TARGET)

backup:
	tar cvf $(TARGET).tar $(PREFIX)*.cpp  $(PREFIX)*.h  *.fld ICONS  Makefile
	gzip $(TARGET).tar

protos:
	cproto -e -q -E 0 -O /dev/null -o protos.h $(IFLAGS) $(PREFIX)*.cc  $(PREFIX)*.c
