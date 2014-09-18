# HTTP SDK Makefile
# Copyright 2004 GameSpy Industries

PROJECT=ghttplinux

CC=gcc
BASE_CFLAGS=-D_LINUX -DGSI_COMMON_DEBUG -D_DEBUG

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -march=i486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
	-falign-jumps=2 -falign-functions=2 -lpthread

#use these when debugging 
DEBUG_CFLAGS=$(BASE_CFLAGS) -g -lpthread -march=i486 -O6

PROG_OBJS = \
	../../../md5c.o\
	../../../darray.o\
	../../../hashtable.o\
	../../../common/linux/LinuxCommon.o\
	../../../common/gsAssert.o\
	../../../common/gsAvailable.o\
	../../../common/gsDebug.o\
	../../../common/gsCrypt.o\
	../../../common/gsLargeInt.o\
	../../../common/gsStringUtil.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/gsMemory.o\
	../../../common/gsXML.o\
	../../../common/gsSHA1.o\
	../../../common/gsRC4.o\
	../../../common/gsSSL.o\
	../../ghttpBuffer.o\
	../../ghttpCallbacks.o\
	../../ghttpConnection.o\
	../../ghttpEncryption.o\
	../../ghttpMain.o\
	../../ghttpProcess.o\
	../../ghttpCommon.o\
	../../ghttpPost.o\
	../ghttpc.o


#############################################################################
# SETUP AND BUILD
#############################################################################

$(PROJECT): $(PROG_OBJS)        
	$(CC) $(DEBUG_CFLAGS) -o $@ $(PROG_OBJS)

#############################################################################
# MISC
#############################################################################
ghttp_debug:  $(PROG_OBJS)        
	$(CC) $(DEBUG_CFLAGS) -o $@ $(PROG_OBJS)

clean:	
	rm -f $(PROG_OBJS)

depend:
	gcc -MM $(PROG_OBJS:.o=.c)

