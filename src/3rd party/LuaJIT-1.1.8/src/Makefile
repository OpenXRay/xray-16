# makefile for building Lua
# see ../INSTALL for installation instructions
# see ../Makefile and luaconf.h for further customization

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

CC= gcc -m32
CFLAGS= -O2 -fomit-frame-pointer -Wall $(MYCFLAGS) $(COCOCFLAGS) $(JITCFLAGS)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS= -lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

# ++ Coco =========
# Default: autodetect gccasm/setjmp/ucontext/fibers context switch method.
COCOCFLAGS=
# Force use of setjmp (instead of gccasm).
#COCOCFLAGS= -DCOCO_USE_SETJMP
# Force use of ucontext (instead of gccasm or setjmp).
#COCOCFLAGS= -DCOCO_USE_UCONTEXT
# Uncomment this if you want to compile LuaJIT without Coco.
# This effectively disables yielding from JIT compiled functions.
#COCOCFLAGS= -DCOCO_DISABLE
# -- Coco =========

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

PLATS= linux bsd macosx solaris mingw cygwin posix generic linux_rl bsd_rl macosx_rl

# ++ Coco =========
COCO_O= lcoco.o
# -- Coco =========

# ++ LuaJIT =========
DASMDIR= ../dynasm
DASMFLAGS=
DASMDISTFLAGS= -LN

# This assumes you already have a copy of (plain) Lua 5.1 installed
# You can use luajit, too (built with the pre-processed headers from the dist)
DASM= lua $(DASMDIR)/dynasm.lua

JITCFLAGS= -I$(DASMDIR)
JIT_O= ljit_core.o ljit_mem.o ljit_dasm.o ljit_backend.o
JITLIB_O=  ljitlib.o

ALL_DH = ljit_x86.h
# -- LuaJIT =========

LUA_A=	liblua.a
CORE_O=	lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o \
	lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o  \
	lundump.o lvm.o lzio.o $(COCO_O) $(JIT_O)
LIB_O=	lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o \
	lstrlib.o loadlib.o $(JITLIB_O) linit.o

# Standalone has been renamed to avoid conflicts during installation
LUA_T=	luajit
LUA_O=	lua.o

LUAC_T=	luac
LUAC_O=	luac.o print.o

ALL_O= $(CORE_O) $(LIB_O) $(LUA_O) $(LUAC_O)
# Do not build luac by default
#ALL_T= $(LUA_A) $(LUA_T) $(LUAC_T)
ALL_T= $(LUA_A) $(LUA_T)
ALL_A= $(LUA_A)

default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

a:	$(ALL_A)

$(LUA_A): $(CORE_O) $(LIB_O)
	$(AR) $@ $(CORE_O) $(LIB_O)
	$(RANLIB) $@

$(LUA_T): $(LUA_O) $(LUA_A)
	$(CC) -o $@ $(MYLDFLAGS) $(LUA_O) $(LUA_A) $(LIBS)

$(LUAC_T): $(LUAC_O) $(LUA_A)
	$(CC) -o $@ $(MYLDFLAGS) $(LUAC_O) $(LUA_A) $(LIBS)

# ++ LuaJIT =========
ljit_x86.h: ljit_x86.dasc ljit_x86_inline.dash ljit_x86.dash
	$(DASM) $(DASMFLAGS) -o $@ ljit_x86.dasc

distclean:	clean
	$(DASM) $(DASMDISTFLAGS) -o ljit_x86.h ljit_x86.dasc

cleaner:	clean
	$(RM) $(ALL_DH)
# -- LuaJIT =========

clean:
	$(RM) $(ALL_T) $(ALL_O)

depend:
	@$(CC) $(CFLAGS) -MM l*.c print.c

echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "MYCFLAGS = $(MYCFLAGS)"
	@echo "MYLDFLAGS = $(MYLDFLAGS)"
	@echo "MYLIBS = $(MYLIBS)"

# convenience targets for popular platforms

none:
	@echo "Please choose a platform:"
	@echo "  $(PLATS)"

bsd:
	$(MAKE) all MYCFLAGS="-DLUA_USE_POSIX -DLUA_USE_DLOPEN" MYLIBS="-Wl,-E"

bsd_rl:
	$(MAKE) all MYCFLAGS="-DLUA_USE_POSIX -DLUA_USE_DLOPEN -DLUA_USE_READLINE" MYLIBS="-Wl,-E -lreadline"

generic:
	$(MAKE) all MYCFLAGS=

linux:
	$(MAKE) all MYCFLAGS=-DLUA_USE_LINUX MYLIBS="-Wl,-E -ldl"

linux_rl:
	$(MAKE) all MYCFLAGS="-DLUA_USE_LINUX -DLUA_USE_READLINE" MYLIBS="-Wl,-E -ldl -lreadline -lhistory -lncurses"

# Mac OS X on Intel Macs only!
macosx:
	$(MAKE) all MYCFLAGS=-DLUA_USE_LINUX
# use this on Mac OS X 10.3
#	$(MAKE) all MYCFLAGS=-DLUA_USE_MACOSX

macosx_rl:
	$(MAKE) all MYCFLAGS="-DLUA_USE_LINUX -DLUA_USE_READLINE" MYLIBS="-lreadline"

mingw:
	$(MAKE) "LUA_A=lua51.dll" "LUA_T=luajit.exe" \
	"AR=$(CC) -shared -o" "RANLIB=strip --strip-unneeded" \
	"MYCFLAGS=-DLUA_BUILD_AS_DLL -maccumulate-outgoing-args" \
	"MYLIBS=" "MYLDFLAGS=-s" luajit.exe

cygwin:
	$(MAKE) "CC=gcc -mno-cygwin" mingw

posix:
	$(MAKE) all MYCFLAGS=-DLUA_USE_POSIX

# Solaris x86 only!
solaris:
	$(MAKE) all MYCFLAGS="-DLUA_USE_POSIX -DLUA_USE_DLOPEN" MYLIBS="-ldl"

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) default o a clean depend echo none cleaner distclean

# DO NOT DELETE

lapi.o: lapi.c lua.h luaconf.h lapi.h lobject.h llimits.h ldebug.h \
  lstate.h ltm.h lzio.h lmem.h lcoco.h ldo.h lfunc.h lgc.h lstring.h \
  ltable.h lundump.h lvm.h
lauxlib.o: lauxlib.c lua.h luaconf.h lauxlib.h
lbaselib.o: lbaselib.c lua.h luaconf.h lauxlib.h lualib.h lcoco.h
lcoco.o: lcoco.c lua.h luaconf.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h lcoco.h ldo.h lvm.h lgc.h
lcode.o: lcode.c lua.h luaconf.h lcode.h llex.h lobject.h llimits.h \
  lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h lcoco.h \
  ldo.h lgc.h ltable.h
ldblib.o: ldblib.c lua.h luaconf.h lauxlib.h lualib.h
ldebug.o: ldebug.c lua.h luaconf.h lapi.h lobject.h llimits.h lcode.h \
  llex.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h \
  lcoco.h ldo.h lfunc.h lstring.h lgc.h ltable.h lvm.h ljit.h
ldo.o: ldo.c lua.h luaconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h lcoco.h ldo.h lfunc.h lgc.h lopcodes.h lparser.h \
  lstring.h ltable.h lundump.h lvm.h ljit.h
ldump.o: ldump.c lua.h luaconf.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h lcoco.h lundump.h
lfunc.o: lfunc.c lua.h luaconf.h lfunc.h lobject.h llimits.h lgc.h lmem.h \
  lstate.h ltm.h lzio.h lcoco.h ljit.h
lgc.o: lgc.c lua.h luaconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h lcoco.h ldo.h lfunc.h lgc.h lstring.h ltable.h
linit.o: linit.c lua.h luaconf.h lualib.h lauxlib.h
liolib.o: liolib.c lua.h luaconf.h lauxlib.h lualib.h
ljit_backend.o: ljit_backend.c lua.h luaconf.h lobject.h llimits.h \
  lstate.h ltm.h lzio.h lmem.h lcoco.h ldo.h lfunc.h lgc.h lstring.h \
  ltable.h lvm.h lopcodes.h ldebug.h ljit.h ljit_hints.h ljit_dasm.h \
  ../dynasm/dasm_proto.h ljit_x86.h
ljit_core.o: ljit_core.c lua.h luaconf.h lobject.h llimits.h lstate.h \
  ltm.h lzio.h lmem.h lcoco.h ldo.h lstring.h lgc.h ltable.h ldebug.h \
  lopcodes.h ljit.h ljit_hints.h luajit.h
ljit_dasm.o: ljit_dasm.c lua.h luaconf.h ljit.h lobject.h llimits.h \
  ljit_dasm.h ../dynasm/dasm_proto.h lmem.h ../dynasm/dasm_x86.h
ljit_mem.o: ljit_mem.c lua.h luaconf.h lmem.h llimits.h ldo.h lobject.h \
  lstate.h ltm.h lzio.h lcoco.h ljit.h ljit_dasm.h ../dynasm/dasm_proto.h
ljitlib.o: ljitlib.c lua.h luaconf.h lauxlib.h luajit.h lualib.h \
  lobject.h llimits.h lstate.h ltm.h lzio.h lmem.h lcoco.h lstring.h \
  lgc.h ltable.h lfunc.h lopcodes.h ljit.h ljit_hints.h
llex.o: llex.c lua.h luaconf.h ldo.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h lcoco.h llex.h lparser.h lstring.h lgc.h ltable.h
lmathlib.o: lmathlib.c lua.h luaconf.h lauxlib.h lualib.h
lmem.o: lmem.c lua.h luaconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h lcoco.h ldo.h
loadlib.o: loadlib.c lua.h luaconf.h lauxlib.h lualib.h luajit.h
lobject.o: lobject.c lua.h luaconf.h ldo.h lobject.h llimits.h lstate.h \
  ltm.h lzio.h lmem.h lcoco.h lstring.h lgc.h lvm.h
lopcodes.o: lopcodes.c lopcodes.h llimits.h lua.h luaconf.h
loslib.o: loslib.c lua.h luaconf.h lauxlib.h lualib.h
lparser.o: lparser.c lua.h luaconf.h lcode.h llex.h lobject.h llimits.h \
  lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h lcoco.h \
  ldo.h lfunc.h lstring.h lgc.h ltable.h
lstate.o: lstate.c lua.h luaconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h lcoco.h ldo.h lfunc.h lgc.h llex.h lstring.h \
  ltable.h ljit.h
lstring.o: lstring.c lua.h luaconf.h lmem.h llimits.h lobject.h lstate.h \
  ltm.h lzio.h lcoco.h lstring.h lgc.h
lstrlib.o: lstrlib.c lua.h luaconf.h lauxlib.h lualib.h
ltable.o: ltable.c lua.h luaconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h lcoco.h ldo.h lgc.h ltable.h
ltablib.o: ltablib.c lua.h luaconf.h lauxlib.h lualib.h
ltm.o: ltm.c lua.h luaconf.h lobject.h llimits.h lstate.h ltm.h lzio.h \
  lmem.h lcoco.h lstring.h lgc.h ltable.h
lua.o: lua.c lua.h luaconf.h lauxlib.h lualib.h luajit.h
luac.o: luac.c lua.h luaconf.h lauxlib.h ldo.h lobject.h llimits.h \
  lstate.h ltm.h lzio.h lmem.h lcoco.h lfunc.h lopcodes.h lstring.h lgc.h \
  lundump.h
lundump.o: lundump.c lua.h luaconf.h ldebug.h lstate.h lobject.h \
  llimits.h ltm.h lzio.h lmem.h lcoco.h ldo.h lfunc.h lstring.h lgc.h \
  lundump.h
lvm.o: lvm.c lua.h luaconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h lcoco.h ldo.h lfunc.h lgc.h lopcodes.h lstring.h ltable.h \
  lvm.h
lzio.o: lzio.c lua.h luaconf.h llimits.h lmem.h lstate.h lobject.h ltm.h \
  lzio.h lcoco.h
print.o: print.c ldebug.h lstate.h lua.h luaconf.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h lcoco.h lopcodes.h lundump.h

# (end of Makefile)
