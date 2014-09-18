# CDKey SDK Server Makefile
# Copyright 2004 GameSpy Industries

PROJECT=sbctest

CC=gcc
BASE_CFLAGS=-D_LINUX

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -march=i486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
	-falign-jumps=2 -falign-functions=2 -lpthread
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

PROG_OBJS = \
	../sbctest.o\
	../../../common/gsMemory.o\
	../../../common/linux/LinuxCommon.o\
	../../../darray.o\
	../../../hashtable.o\
	../../../common/gsAvailable.o\
	../../../common/gsStringUtil.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../sb_crypt.o\
	../../sb_queryengine.o\
	../../sb_server.o\
	../../sb_serverbrowsing.o\
	../../sb_serverlist.o\
	../../../natneg/natneg.o\
    ../../../natneg/NATify.o\
	../../../qr2/qr2regkeys.o


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

