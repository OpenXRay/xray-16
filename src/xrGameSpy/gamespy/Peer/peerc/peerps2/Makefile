#GameSpy.net PS2 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DUNIQUEID

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 

#Name of the SDK sample
TARGET      = peerc

#All the object files needed for this SDK
OBJS        = ../../../ps2common/ps2common.o \
              ../../../nonport.o\
              ../../../darray.o\
              ../../../md5c.o\
              ../../../hashtable.o\
              ../../../available.o\
              ../../../stringutil.o\
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
              crt0.o\
              ../$(TARGET).o

#Include the stuff common to the GameSpy.net SDKs
include ../../../ps2common/Makefile.common
