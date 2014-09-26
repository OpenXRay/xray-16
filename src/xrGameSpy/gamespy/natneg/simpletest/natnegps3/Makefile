#GameSpy.net PS2 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CLFAGS  = 

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 

#Name of the SDK sample
TARGET      = simpletest

#All the object files needed for this SDK
OBJS        = ../../../common/ps3/ps3common.o \
              ../../../common/gsPlatform.o \
              ../../../common/gsPlatformSocket.o \
              ../../../common/gsPlatformThread.o \
              ../../../common/gsPlatformUtil.o \
              ../../../common/gsDebug.o \
              ../../../common/gsAvailable.o \
              ../../../common/gsStringUtil.o \
              ../../../darray.o\
              ../../natneg.o\
              ../$(TARGET).o

#Include the stuff common to the GameSpy.net SDKs
include ../../../common/ps3/Makefile.common
