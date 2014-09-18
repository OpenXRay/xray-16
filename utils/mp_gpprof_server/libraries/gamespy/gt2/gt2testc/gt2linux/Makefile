# GameSpy Transport 2 SDK Makefile
# Copyright 2004 GameSpy Industries

TARGET=gt2testc

CC=gcc
BASE_CFLAGS=-D_LINUX

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -m486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
	-falign-jumps=2 -falign-functions=2 -lpthread
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

PROG_OBJS = \
	../../../darray.o\
	../../../hashtable.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/gsStringUtil.o\
	../../../common/gsDebug.o\
	../../../common/gsMemory.o\
	../../../common/linux/LinuxCommon.o\
	../../gt2Auth.o\
	../../gt2Buffer.o\
	../../gt2Callback.o\
	../../gt2Connection.o\
	../../gt2Filter.o\
	../../gt2Main.o\
	../../gt2Message.o\
	../../gt2Socket.o\
	../../gt2Encode.o\
	../../gt2Utility.o\
	../$(TARGET).o

#############################################################################
# SETUP AND BUILD
#############################################################################

$(TARGET): $(PROG_OBJS)        
	$(CC) $(CFLAGS) -o $@ $(PROG_OBJS)

#############################################################################
# MISC
#############################################################################

clean:	
	rm -f $(PROG_OBJS)

depend:
	gcc -MM $(PROG_OBJS:.o=.c)

