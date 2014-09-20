# Peer SDK Makefile
# Copyright 2004 GameSpy Industries

PROJECT=peersdk

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
	../../../md5c.o\
	../../../hashtable.o\
	../../../common/gsAvailable.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/gsStringUtil.o\
	../../../common/gsDebug.o\
	../../../common/gsMemory.o\
	../../../common/linux/LinuxCommon.o\
	../../../Chat/chatCallbacks.o\
	../../../Chat/chatChannel.o\
	../../../Chat/chatCrypt.o\
	../../../Chat/chatHandlers.o\
	../../../Chat/chatMain.o\
	../../../Chat/chatSocket.o\
	../../../pinger/pingerMain.o\
	../../../serverbrowsing/sb_crypt.o\
	../../../serverbrowsing/sb_queryengine.o\
	../../../serverbrowsing/sb_server.o\
	../../../serverbrowsing/sb_serverbrowsing.o\
	../../../serverbrowsing/sb_serverlist.o\
	../../../qr2/qr2.o\
	../../../qr2/qr2regkeys.o\
        ../../../natneg/natneg.o\
        ../../../natneg/NATify.o\
	../../peerAutoMatch.o\
	../../peerCallbacks.o\
	../../peerGlobalCallbacks.o\
	../../peerHost.o\
	../../peerKeys.o\
	../../peerMain.o\
	../../peerMangle.o\
	../../peerOperations.o\
	../../peerPing.o\
	../../peerPlayers.o\
	../../peerQR.o\
	../../peerRooms.o\
	../../peerSB.o\
	../peerc.o


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

