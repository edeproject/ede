#
# $Id$
#
# Part of Equinox Desktop Environment (EDE).
# Copyright (c) 2000-2006 EDE Authors.
#
# This program is licenced under terms of the 
# GNU General Public Licence version 2 or newer.
# See COPYING for details.

include ../../makeinclude

define make_test
ALL += $1
$1 : $1.cpp
	echo Compiling $$<...
	$(CXX) $(CXXFLAGS) $$< -o $$@ $(LIBS)
endef

# Note: $(eval...) must be used instead plain $(call...)
# since variables will not be expanded correctly

$(eval $(call make_test, basic_moveresize))

sound_test.o : sound_test.cpp 
	echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $<
Sound.o : ../Sound.cpp
	echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $<
sound_test: sound_test.o Sound.o
	$(CXX) sound_test.o Sound.o ../debug.o -o $@ $(LIBS)

all: $(ALL) sound_test

clean:
	$(RM) $(ALL)
	$(RM) *.o
