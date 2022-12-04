CC = g++
CFLAGS = $(INCLUDE_DIRS) -Wall -Wextra -ansi -pedantic -g -no-pie -std=c++11
CYGWIN_OPTS = -Wl,--enable-auto-import
LDLIBS = -lGL -lGLEW -lglfw
BUILDDIR = ../build
EXECS = module_03_milestone

all : $(EXECS) postbuild

module_03_milestone : module_03_milestone.cpp
	$(CC) $(CFLAGS) -o module_03_milestone module_03_milestone.cpp $(LDLIBS)


$(BUILDDIR) :
	mkdir $(BUILDDIR)
	mkdir $(BUILDDIR)/linux

postbuild: | $(BUILDDIR)
	mv $(EXECS) $(BUILDDIR)/linux

clean :
	if [ -d $(BUILDDIR) ]; then \
        	cd $(BUILDDIR); \
        	rm $(EXECS); \
    	fi


