###############################################################################
# The following macros should be defined before this script is
# invoked:
#
# TOP              The toplevel directory of the source tree.  
#
# BCC              C Compiler and options for use in building executables that
#                  will run on the platform that is doing the build.
#
# OPTS             Extra compiler command-line options.
#
# EXE              The suffix to add to executable files.  ".exe" for windows
#                  and "" for Unix.
#
# TCC              C Compiler and options for use in building executables that 
#                  will run on the target platform.  This is usually the same
#                  as BCC, unless you are cross-compiling.
#
# AR               Tools used to build a static library.
# RANLIB
#
# TCL_FLAGS        Extra compiler options needed for programs that use the
#                  TCL library.
#
# LIBTCL           Linker options needed to link against the TCL library.
#
# Once the macros above are defined, the rest of this make script will
# build the product
################################################################################

# This is how we compile
#
TCCX =  $(TCC) $(OPTS) -I. -I$(TOP)/src -I$(TOP)
TCCX += -DSQLITE_OMIT_LOAD_EXTENSION
TCCX += -DSQLITE_THREADSAFE=0

# Object files for the XJD1 library.
#
LIBOBJ+= complete.o conn.o context.o
LIBOBJ+= datasrc.o delete.o
LIBOBJ+= expr.o
LIBOBJ+= func.o
LIBOBJ+= json.o
LIBOBJ+= memory.o
LIBOBJ+= parse.o pragma.o
LIBOBJ+= query.o
LIBOBJ+= sqlite3.o stmt.o string.o
LIBOBJ+= tokenize.o trace.o
LIBOBJ+= update.o

# All of the source code files.
#
SRC = \
  $(TOP)/src/parse.y 

# Generated source code files
#
SRC += \
  main.c \
  parse.c \
  parse.y \
  xjd1.h \
  xjd1Int.h

# Header files used by all library source files.
#
HDR = \
   parse.h \
   parse_txt.h \
   $(TOP)/src/xjd1.h \
   $(TOP)/src/xjd1Int.h

# This is the default Makefile target.
#
all:	xjd1


# Run all tests
#
test:	xjd1
	./xjd1 $(TOP)/test/all.test

# The shell program
#
xjd1:	libxjd1.a $(TOP)/src/shell.c
	$(TCCX) -o xjd1 $(TOP)/src/shell.c libxjd1.a $(AUXLIB) -lm

# The library
#
libxjd1.a:	$(LIBOBJ)
	$(AR) libxjd1.a $(LIBOBJ)
	$(RANLIB) libxjd1.a

# Rules to build the LEMON compiler generator
#
lemon:	$(TOP)/tool/lemon.c $(TOP)/src/lempar.c
	$(BCC) -o lemon $(TOP)/tool/lemon.c
	cp $(TOP)/src/lempar.c .

# Rules to build individual *.o files from generated *.c files. This
# applies to:
#
#     parse.o
#
parse.o: parse.c $(HDR)
	$(TCCX) -c $<

# Construct the parse_txt.h file from parse.h
#
parse_txt.h:	parse.c $(TOP)/mkparsetxth.awk
	awk -f $(TOP)/mkparsetxth.awk <parse.h >parse_txt.h

# Rules to build individual *.o files from files in the src directory.
#
%.o: $(TOP)/src/%.c $(HDR)
	$(TCCX) -c $<

# Rules to build parse.c and parse.h - the outputs of lemon.
#
parse.h:	parse.c

parse.c:	$(TOP)/src/parse.y lemon
	cp $(TOP)/src/parse.y .
	rm -f parse.h
	./lemon $(OPTS) parse.y

sqlite3.o:	$(TOP)/src/sqlite3.c $(TOP)/src/sqlite3.h
	$(TCCX) -c -DSQLITE_NO_SYNC -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOADEXTENSION $<

clean:	
	rm -f *.o lib*.a
	rm -f lemon xjd1 parse.* parse_txt.h lempar.c
