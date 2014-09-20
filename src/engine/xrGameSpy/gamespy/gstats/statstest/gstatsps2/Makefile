#GameSpy.net PS2 Makefile

#SDK-specific compiler flags
#If the SDK uses Unique IDs, add "-DUNIQUEID"
SDK_CLFAGS  = 

#SDK-specific libraries
#If the SDK needs Logitech audio libraries, add "$(LIBDIR)/liblgaud.a"
SDK_LIBS    = 

#Name of the SDK sample
TARGET      = statstest

#All the object files needed for this SDK
OBJS        = ../../../ps2common/ps2common.o \
              ../../../nonport.o \
              ../../../darray.o \
              ../../../hashtable.o \
              ../../../md5c.o \
              ../../../stringutil.o \
              ../../../available.o \
              ../../gstats.o \
              ../../gbucket.o \
              crt0.o \
              ../$(TARGET).o

#Include the stuff common to the GameSpy.net SDKs
include ../../../ps2common/Makefile.common
