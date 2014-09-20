# Patching & Tracking SDK Makefile
# Copyright 2004 GameSpy Industries

PROJECT=pt

CC=gcc
BASE_CFLAGS=-D_LINUX

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -m486 -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
	-falign-jumps=2 -falign-functions=2 -lpthread
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

PROG_OBJS = \
	../pttestc.o\
	../../../md5c.o\
	../../../darray.o\
	../../../hashtable.o\
	../../../common/gsAvailable.o\
	../../../common/gsCrypt.o\
	../../../common/gsLargeInt.o\
	../../../common/gsStringUtil.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/gsDebug.o\
	../../../common/gsXML.o\
	../../../common/gsRC4.o\
	../../../common/gsSHA1.o\
	../../../common/gsSSL.o\
	../../../common/gsMemory.o\
	../../../common/linux/LinuxCommon.o\
	../../ptMain.o\
	../../../ghttp/ghttpBuffer.o\
	../../../ghttp/ghttpCallbacks.o\
	../../../ghttp/ghttpCommon.o\
	../../../ghttp/ghttpConnection.o\
	../../../ghttp/ghttpEncryption.o\
	../../../ghttp/ghttpMain.o\
	../../../ghttp/ghttpPost.o\
	../../../ghttp/ghttpProcess.o\

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

