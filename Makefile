# Path Converter

# Resulting executable's name
PROG = pathconv
RELEASE_FILES = README.txt TODO.txt
# Directory to put executable and release files in
BINDIR = release
# Directory to put object files in
OBJDIR = objs
# Directory to install program to
INSTALLPROG = /usr/bin
# gcc params
LIBS = 
FLAGS = -O2

### DON'T TOUCH BELOW THIS POINT ###
## Generic Makefile by Wa (logicplace.com) CAUSE I ALWAYS FORGET HOW THESE WORK ##

CWD := $(shell pwd)

SOURCES = $(wildcard *.c) 
OBJS = $(SOURCES:%.c=$(OBJDIR)/%.o)

$(PROG): $(OBJS)
	gcc $(FLAGS) $(OBJS) -o $(BINDIR)/$(PROG) $(LIBS:%=-l%)
ifdef RELEASE_FILES
	cp -u $(RELEASE_FILES) $(BINDIR)/
endif

$(OBJS): $(OBJDIR)/%.o : %.c $(BINDIR) $(OBJDIR)
	gcc $(FLAGS) -c $< -o $@

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

install: $(BINDIR)/$(PROG)
	cp $(BINDIR)/$(PROG) $(INSTALLPROG)/$(PROG)

install-ln: $(BINDIR)/$(PROG)
	ln -s $(CWD)/$(BINDIR)/$(PROG) $(INSTALLPROG)/$(PROG)

clean: 
	rm $(OBJS)

revert:
	rm -r $(OBJDIR)
	rm -r $(BINDIR)

