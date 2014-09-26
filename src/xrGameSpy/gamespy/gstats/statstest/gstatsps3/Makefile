#GameSpy.net PS3 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DGSI_COMMON_DEBUG

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 
              

#Name of the SDK sample
TARGET      = statstest

#All the object files needed for this SDK
OBJS        = ../../../common/ps3/ps3common.o \
              ../../../common/gsPlatform.o\
              ../../../common/gsPlatformSocket.o \
              ../../../common/gsPlatformThread.o \
              ../../../common/gsPlatformUtil.o \
              ../../../common/gsAvailable.o \
              ../../../common/gsMemory.o \
              ../../../common/gsDebug.o \
              ../../../common/gsStringUtil.o \
              ../../../hashtable.o \
              ../../../darray.o \
              ../../../md5c.o \
              ../../gstats.o \
              ../../gbucket.o \
              ../$(TARGET).o

#Include the stuff common to the GameSpy.net SDKs
include ../../../common/ps3/Makefile.common
