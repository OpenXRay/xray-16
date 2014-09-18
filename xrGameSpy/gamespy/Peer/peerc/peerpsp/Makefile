#GameSpy.net PSP Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DUNIQUEID

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
#SDK_LIBS    = 

#Name of the SDK sample
TARGET      = peerc

#All the object files needed for this SDK
OBJS	= ../../../common/psp/pspcommon.o\
          ../../../common/psp/gsUtilPSP.o\
          ../../../common/gsPlatformSocket.o\
          ../../../common/gsPlatformUtil.o\
          ../../../common/gsStringUtil.o\
          ../../../common/gsMemory.o\
          ../../../common/gsDebug.o\
          ../../../common/gsAvailable.o\
          ../../../md5c.o\
          ../../../darray.o\
          ../../../hashtable.o\
          ../../../pinger/pingerMain.o\
          ../../../qr2/qr2.o\
          ../../../qr2/qr2regkeys.o\
          ../../../serverbrowsing/sb_crypt.o\
          ../../../serverbrowsing/sb_queryengine.o\
          ../../../serverbrowsing/sb_server.o\
          ../../../serverbrowsing/sb_serverbrowsing.o\
          ../../../serverbrowsing/sb_serverlist.o\
          ../../../chat/chatCallbacks.o\
          ../../../chat/chatChannel.o\
          ../../../chat/chatCrypt.o\
          ../../../chat/chatHandlers.o\
          ../../../chat/chatMain.o\
          ../../../chat/chatSocket.o\
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
          ../$(TARGET).o
	  
#Include the stuff common to the GameSpy.net SDKs
include ../../../common/psp/Makefile.common

