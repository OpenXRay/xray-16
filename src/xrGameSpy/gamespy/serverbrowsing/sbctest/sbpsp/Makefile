#GameSpy.net PSP Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DUNIQUEID -DGSI_MEM_MANAGED -DGSI_COMMON_DEBUG

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
#SDK_LIBS    = 

#Name of the SDK sample
TARGET      = sbctest

#All the object files needed for this SDK
OBJS	= ../../../common/psp/pspcommon.o\
          ../../../common/psp/gsUtilPSP.o\
          ../../../common/gsPlatformSocket.o\
          ../../../common/gsPlatformUtil.o\
          ../../../common/gsStringUtil.o\
          ../../../common/gsAvailable.o\
          ../../../common/gsMemory.o\
          ../../../common/gsDebug.o\
          ../../sb_crypt.o\
          ../../sb_queryengine.o\
          ../../sb_serverbrowsing.o\
          ../../sb_serverlist.o\
          ../../sb_server.o\
          ../../../hashtable.o\
          ../../../darray.o\
          ../../../qr2/qr2regkeys.o\
          ../$(TARGET).o
	  
#Include the stuff common to the GameSpy.net SDKs
include ../../../common/psp/Makefile.common

