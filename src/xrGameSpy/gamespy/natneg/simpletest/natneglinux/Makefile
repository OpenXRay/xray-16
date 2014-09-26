# NAT Negotiation SDK Makefile
# Copyright 2004 GameSpy Industries

PROJECT=natneg

CC=gcc
BASE_CFLAGS=-D_LINUX -D_DEBUG -DGSI_COMMON_DEBUG

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -m486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
	-falign-jumps=2 -falign-functions=2 -lpthread
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

PROG_OBJS = \
	../../../darray.o\
	../../../common/gsAssert.o\
	../../../common/gsAvailable.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/gsStringUtil.o\
	../../../common/gsDebug.o\
	../../../common/gsMemory.o\
	../../../common/linux/LinuxCommon.o\
	../../natneg.o\
	../../NATify.o\
	../simpletest.o


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

