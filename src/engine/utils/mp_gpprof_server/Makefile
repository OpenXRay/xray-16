SHELL		= /bin/sh
ROOT_DIR	= .
BIN_DIRECTORY	= $(ROOT_DIR)/bin
LIBS_DIR	= $(ROOT_DIR)/libraries
FASTCGI_LIB_DIR	= $(LIBS_DIR)/fcgi-2.4.0
GAMESPY_LIB_DIR	= $(LIBS_DIR)/gamespy
TARGET_NAME	= $(BIN_DIRECTORY)/mp_gpprof_server

INSTALL_DIR	= /usr/share/mp_gpprof_server
INITD_SCRIPT	= mpgpprof
INITD_DIR	= /etc/init.d

CC		= gcc
CPP		= g++

BASE_CFLAGS=-D_LINUX -DGSI_NO_THREADS

CFLAGS=$(BASE_CFLAGS) -O6 -ffast-math -funroll-loops \
        -fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
        -falign-jumps=2 -falign-functions=2 -lpthread
CPPFLAGS=$(BASE_CFLAGS) -march=i486 -lpthread

DEBUG_CFLAGS	=$(BASE_CFLAGS) -g -lpthread -DGSI_COMMON_DEBUG -D_DEBUG
DEBUG_CPPFLAGS	=$(BASE_CFLAGS) -g -lpthread -DGSI_COMMON_DEBUG -D_DEBUG


#use these to specify debug and release configurations
CURR_CFLAGS	= $(CFLAGS)
CURR_CPPFLAGS	= $(CPPFLAGS)

FASTCGI_OBJS 	=\
	$(BIN_DIRECTORY)/libfcgi/fcgiapp.o\
	$(BIN_DIRECTORY)/libfcgi/os_unix.o\
	$(BIN_DIRECTORY)/libfcgi/fcgi_stdio.o\
	$(BIN_DIRECTORY)/libfcgi/fcgio.o

MP_GPPROF_OBJS = \
	$(BIN_DIRECTORY)/entry_point.o\
	$(BIN_DIRECTORY)/gamespy_sake.o\
	$(BIN_DIRECTORY)/profile_data_types.o\
	$(BIN_DIRECTORY)/profile_request.o\
	$(BIN_DIRECTORY)/profiles_cache.o\
	$(BIN_DIRECTORY)/requests_processor.o\
	$(BIN_DIRECTORY)/sake_worker.o\
	$(BIN_DIRECTORY)/threads.o\
	$(BIN_DIRECTORY)/atlas_stalkercoppc_v1.o

GAMESPY_OBJS		=\
	$(BIN_DIRECTORY)/gamespy/md5c.o\
	$(BIN_DIRECTORY)/gamespy/darray.o\
	$(BIN_DIRECTORY)/gamespy/hashtable.o\
	$(BIN_DIRECTORY)/gamespy/common/linux/LinuxCommon.o\
	$(BIN_DIRECTORY)/gamespy/common/gsAssert.o\
	$(BIN_DIRECTORY)/gamespy/common/gsAvailable.o\
	$(BIN_DIRECTORY)/gamespy/common/gsDebug.o\
	$(BIN_DIRECTORY)/gamespy/common/gsCore.o\
	$(BIN_DIRECTORY)/gamespy/common/gsCrypt.o\
	$(BIN_DIRECTORY)/gamespy/common/gsLargeInt.o\
	$(BIN_DIRECTORY)/gamespy/common/gsStringUtil.o\
	$(BIN_DIRECTORY)/gamespy/common/gsPlatform.o\
	$(BIN_DIRECTORY)/gamespy/common/gsPlatformSocket.o\
	$(BIN_DIRECTORY)/gamespy/common/gsPlatformThread.o\
	$(BIN_DIRECTORY)/gamespy/common/gsPlatformUtil.o\
	$(BIN_DIRECTORY)/gamespy/common/gsMemory.o\
	$(BIN_DIRECTORY)/gamespy/common/gsXML.o\
	$(BIN_DIRECTORY)/gamespy/common/gsSHA1.o\
	$(BIN_DIRECTORY)/gamespy/common/gsRC4.o\
	$(BIN_DIRECTORY)/gamespy/common/gsSSL.o\
	$(BIN_DIRECTORY)/gamespy/common/gsSoap.o\
	$(BIN_DIRECTORY)/gamespy/common/gsUdpEngine.o\
	$(BIN_DIRECTORY)/gamespy/ghttp/ghttpBuffer.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpCallbacks.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpConnection.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpEncryption.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpMain.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpProcess.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpCommon.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpPost.o\
        $(BIN_DIRECTORY)/gamespy/ghttp/ghttpc/ghttpc.o\
	$(BIN_DIRECTORY)/gamespy/GP/gp.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpi.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiBuddy.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiBuffer.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiCallback.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiConnect.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiInfo.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiKeys.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiOperation.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiPeer.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiProfile.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiSearch.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiUnique.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiUtility.o\
        $(BIN_DIRECTORY)/gamespy/GP/gpiTransfer.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Auth.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Buffer.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Callback.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Connection.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Encode.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Filter.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Main.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Message.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Socket.o\
        $(BIN_DIRECTORY)/gamespy/gt2/gt2Utility.o\
	$(BIN_DIRECTORY)/gamespy/sake/sakeMain.o\
	$(BIN_DIRECTORY)/gamespy/sake/sakeRequest.o\
	$(BIN_DIRECTORY)/gamespy/sake/sakeRequestMisc.o\
	$(BIN_DIRECTORY)/gamespy/sake/sakeRequestModify.o\
	$(BIN_DIRECTORY)/gamespy/sake/sakeRequestRead.o



$(TARGET_NAME): $(GAMESPY_OBJS) $(FASTCGI_OBJS) $(MP_GPPROF_OBJS)
	$(CPP) $(CURR_CPPFLAGS) $(GAMESPY_OBJS) $(FASTCGI_OBJS) $(MP_GPPROF_OBJS) -o $@

$(BIN_DIRECTORY)/%.o: %.c
	$(CC) $(CURR_CFLAGS) -I$(FASTCGI_LIB_DIR) -I$(FASTCGI_LIB_DIR)/include -I$(GAMESPY_LIB_DIR) -c $< -o $@

$(BIN_DIRECTORY)/%.o: %.cpp
	$(CPP) $(CURR_CPPFLAGS) -I$(FASTCGI_LIB_DIR) -I$(FASTCGI_LIB_DIR)/include -I$(GAMESPY_LIB_DIR) -c $< -o $@

$(BIN_DIRECTORY)/gamespy/%.o: $(GAMESPY_LIB_DIR)/%.c
	$(CC) $(CURR_CFLAGS) -I$(FASTCGI_LIB_DIR) -I$(FASTCGI_LIB_DIR)/include -I$(GAMESPY_LIB_DIR) -c $< -o $@

$(BIN_DIRECTORY)/libfcgi/%.o: $(FASTCGI_LIB_DIR)/libfcgi/%.c
	$(CC) $(CURR_CFLAGS) -I$(FASTCGI_LIB_DIR) -I$(FASTCGI_LIB_DIR)/include -I$(GAMESPY_LIB_DIR) -c $< -o $@

$(BIN_DIRECTORY)/libfcgi/%.o: $(FASTCGI_LIB_DIR)/libfcgi/%.cpp
	$(CPP) $(CURR_CFLAGS) -I$(FASTCGI_LIB_DIR) -I$(FASTCGI_LIB_DIR)/include -I$(GAMESPY_LIB_DIR) -c $< -o $@


clean:
	find ./ -name "*.o" | xargs rm -f
	rm -f $(TARGET_NAME)
all: $(TARGET_NAME)

install: $(TARGET_NAME)
	mkdir $(INSTALL_DIR)
	cp $(TARGET_NAME) $(INSTALL_DIR)/mp_gpprof_server
	cp $(ROOT_DIR)/$(INITD_SCRIPT) $(INITD_DIR)/$(INITD_SCRIPT)
	chkconfig -a $(INITD_SCRIPT)
	$(INITD_DIR)/$(INITD_SCRIPT) start

uninstall:
	$(INITD_DIR)/$(INITD_SCRIPT) stop
	rm -f $(INSTALL_DIR)/mp_gpprof_server
	rmdir $(INSTALL_DIR)
	rm -f $(INITD_DIR)/$(INITD_SCRIPT)
