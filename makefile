#
# Makefile for 'comments'
#

CC	= gcc -c
CFLAGS	= -O2 -Zmtd
LD	= gcc
LFLAGS	= -s -Zmtd

#
# Inference Rules
#
.c.o :
	$(CC) $(CFLAGS) $*.c

#
# Files to use
#

HDRS =

SRCS = comments.c

OBJS = comments.o

#
# Target to build
#

all : comments.exe

#
# Object Modules
#

comments.o : comments.c

#
# Executable
#

comments.exe : comments.def $(OBJS)
	$(LD) $(LDFLAGS) -o comments.exe comments.def $(OBJS)
