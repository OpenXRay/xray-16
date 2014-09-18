# CDKey SDK Server
# Copyright 2004 GameSpy Industries

PROJECT=gcdkeyserver

CC=gcc
BASE_CFLAGS=-D_LINUX -I../ -I../..

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -mtune=i486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
	-falign-jumps=2 -falign-functions=2 -lpthread
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

PROG_OBJS = \
	../../gcdkeys.o\
	../../../md5c.o\
	../../../common/gsAvailable.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/gsStringUtil.o\
	../../../common/gsDebug.o\
	../../../common/gsMemory.o\
	../../../common/linux/LinuxCommon.o\
	../../ServerTest/gcdkeyservertest.o\


#############################################################################
# SETUP AND BUILD
#############################################################################

$(PROJECT): $(PROG_OBJS)        
	$(CC) $(CFLAGS) -o $@ $(PROG_OBJS)

#############################################################################
# MISC
#############################################################################

clean:	
	rm -f $(PROG_OBJS)

depend:
	gcc -MM $(PROG_OBJS:.o=.c)

