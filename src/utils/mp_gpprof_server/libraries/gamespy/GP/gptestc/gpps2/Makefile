#GameSpy.net PS2 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CLFAGS  = 

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 

#Name of the SDK sample
TARGET      = gptestc

#All the object files needed for this SDK
OBJS        = ../../../ps2common/ps2common.o \
              ../../../nonport.o \
              ../../../md5c.o \
              ../../../darray.o\
              ../../../hashtable.o\
              ../../../stringutil.o\
              ../../../available.o\
              ../../gp.o \
              ../../gpi.o \
              ../../gpiBuddy.o \
              ../../gpiBuffer.o \
              ../../gpiCallback.o \
              ../../gpiConnect.o \
              ../../gpiInfo.o \
              ../../gpiOperation.o \
              ../../gpiPeer.o \
              ../../gpiProfile.o \
              ../../gpiSearch.o \
              ../../gpiUnique.o \
              ../../gpiUtility.o \
              ../../gpiTransfer.o \
              crt0.o\
              ../$(TARGET).o

#Include the stuff common to the GameSpy.net SDKs
include ../../../ps2common/Makefile.common
