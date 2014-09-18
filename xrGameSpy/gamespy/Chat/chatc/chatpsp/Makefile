#GameSpy.net PSP Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DUNIQUEID

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
#SDK_LIBS    = 

#Name of the SDK sample
TARGET      = chatc

#All the object files needed for this SDK
OBJS	= ../../../common/psp/pspcommon.o\
          ../../../common/psp/gsUtilPSP.o\
          ../../../common/gsPlatformSocket.o\
          ../../../common/gsPlatformUtil.o\
          ../../../common/gsStringUtil.o\
          ../../../md5c.o\
          ../../../darray.o\
          ../../../hashtable.o\
          ../../chatCallbacks.o\
          ../../chatChannel.o\
          ../../chatCrypt.o\
          ../../chatHandlers.o\
          ../../chatMain.o\
          ../../chatSocket.o\
          ../$(TARGET).o
	  
#Include the stuff common to the GameSpy.net SDKs
include ../../../common/psp/Makefile.common

