#GameSpy.net PS2 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CFLAGS  = -DUNIQUEID

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 

#Name of the SDK sample
TARGET      = pttestc

#All the object files needed for this SDK
OBJS        = ../../../ps2common/ps2common.o \
              ../../../nonport.o \
              ../../../darray.o\
              ../../../stringutil.o \
              ../../../available.o \
              ../../ptMain.o \
              ../../../ghttp/ghttpBuffer.o \
              ../../../ghttp/ghttpCallbacks.o \
              ../../../ghttp/ghttpConnection.o \
              ../../../ghttp/ghttpMain.o \
              ../../../ghttp/ghttpProcess.o \
              ../../../ghttp/ghttpCommon.o \
              ../../../ghttp/ghttpPost.o \
              crt0.o\
              ../$(TARGET).o

#Include the stuff common to the GameSpy.net SDKs
include ../../../ps2common/Makefile.common
