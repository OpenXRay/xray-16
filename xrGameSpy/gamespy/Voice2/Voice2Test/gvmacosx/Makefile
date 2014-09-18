# Voice2 SDK test app Makefile - Mac OSX
# Copyright 2006 GameSpy Industries

PROJECT=voice2test

SPEEX=../../speex-1.0.5/
SPEEX_SRC=$(SPEEX)libspeex/
SPEEX_INC=$(SPEEX)include/

INCLUDES=-I$(SPEEX_INC)

DEFINES=-DGV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN

LIBS=-framework CoreAudio -framework AudioToolbox -framework CoreFoundation

PROG_OBJS = \
	../../../hashtable.o\
	../../../darray.o\
	../../../md5c.o\
	../../../common/gsMemory.o\
	../../../common/gsAvailable.o\
	../../../common/gsDebug.o\
	../../../common/gsStringUtil.o\
	../../../common/gsPlatform.o\
	../../../common/gsPlatformSocket.o\
	../../../common/gsPlatformThread.o\
	../../../common/gsPlatformUtil.o\
	../../../common/macosx/MacOSXCommon.o\
	$(SPEEX_SRC)bits.o\
	$(SPEEX_SRC)cb_search.o\
	$(SPEEX_SRC)exc_10_16_table.o\
	$(SPEEX_SRC)exc_10_32_table.o\
	$(SPEEX_SRC)exc_20_32_table.o\
	$(SPEEX_SRC)exc_5_256_table.o\
	$(SPEEX_SRC)exc_5_64_table.o\
	$(SPEEX_SRC)exc_8_128_table.o\
	$(SPEEX_SRC)filters.o\
	$(SPEEX_SRC)gain_table.o\
	$(SPEEX_SRC)gain_table_lbr.o\
	$(SPEEX_SRC)hexc_10_32_table.o\
	$(SPEEX_SRC)hexc_table.o\
	$(SPEEX_SRC)high_lsp_tables.o\
	$(SPEEX_SRC)lpc.o\
	$(SPEEX_SRC)lsp.o\
	$(SPEEX_SRC)lsp_tables_nb.o\
	$(SPEEX_SRC)ltp.o\
	$(SPEEX_SRC)math_approx.o\
	$(SPEEX_SRC)misc.o\
	$(SPEEX_SRC)modes.o\
	$(SPEEX_SRC)nb_celp.o\
	$(SPEEX_SRC)quant_lsp.o\
	$(SPEEX_SRC)sb_celp.o\
	$(SPEEX_SRC)speex_callbacks.o\
	$(SPEEX_SRC)speex_header.o\
	$(SPEEX_SRC)stereo.o\
	$(SPEEX_SRC)vbr.o\
	$(SPEEX_SRC)vq.o\
	../../gvCodec.o\
	../../gvCustomDevice.o\
	../../gvDevice.o\
	../../gvFrame.o\
	../../gvMain.o\
	../../gvOSXAudio.o\
	../../gvSource.o\
	../../gvSpeex.o\
	../../gvUtil.o\
	../Voice2Test.o

#Include the stuff common to the GameSpy.net SDKs
include ../../../common/macosx/Makefile.common



