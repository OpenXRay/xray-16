#GameSpy.net PS3 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DGSI_COMMON_DEBUG -DUNIQUEID

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 
              

#Name of the SDK sample
TARGET      = peerc

#All the object files needed for this SDK
OBJS        = ../../../common/ps3/ps3common.o \
              ../../../common/gsPlatform.o\
              ../../../common/gsPlatformSocket.o \
              ../../../common/gsPlatformThread.o \
              ../../../common/gsPlatformUtil.o \
              ../../../common/gsAvailable.o \
              ../../../common/gsDebug.o \
              ../../../common/gsMemory.o \
              ../../../common/gsStringUtil.o \
              ../../../serverbrowsing/sb_crypt.o\
              ../../../serverbrowsing/sb_queryengine.o\
              ../../../serverbrowsing/sb_server.o\
              ../../../serverbrowsing/sb_serverbrowsing.o\
              ../../../serverbrowsing/sb_serverlist.o\
              ../../../qr2/qr2.o\
              ../../../qr2/qr2regkeys.o\
              ../../../Chat/chatCrypt.o\
              ../../../Chat/chatCallbacks.o\
              ../../../Chat/chatChannel.o\
              ../../../Chat/chatHandlers.o\
              ../../../Chat/chatMain.o\
              ../../../Chat/chatSocket.o\
              ../../../pinger/pingerMain.o\
              ../../../hashtable.o\
              ../../../darray.o\
              ../../../md5c.o\
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
include ../../../common/ps3/Makefile.common
