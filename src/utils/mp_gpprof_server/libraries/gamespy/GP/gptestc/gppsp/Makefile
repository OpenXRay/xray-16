#GameSpy.net PSP Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DUNIQUEID -DGSI_MEM_MANAGED -DGSI_COMMON_DEBUG

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
#SDK_LIBS    = 

#Name of the SDK sample
TARGET      = gptestc

#All the object files needed for this SDK
OBJS	= ../../../common/psp/pspcommon.o\
          ../../../common/psp/gsUtilPSP.o\
          ../../../common/gsPlatformSocket.o\
          ../../../common/gsPlatformUtil.o\
          ../../../common/gsStringUtil.o\
          ../../../common/gsMemory.o\
          ../../../common/gsAvailable.o\
          ../../../common/gsDebug.o\
          ../../../darray.o\
          ../../../hashtable.o\
          ../../../md5c.o\
          ../../gp.o\
          ../../gpi.o\
          ../../gpiBuddy.o\
          ../../gpiBuffer.o\
          ../../gpiCallback.o\
          ../../gpiConnect.o\
          ../../gpiInfo.o\
          ../../gpiOperation.o\
          ../../gpiPeer.o\
          ../../gpiProfile.o\
          ../../gpiSearch.o\
          ../../gpiTransfer.o\
          ../../gpiUnique.o\
          ../../gpiUtility.o\
          ../$(TARGET).o
	  
#Include the stuff common to the GameSpy.net SDKs
include ../../../common/psp/Makefile.common

