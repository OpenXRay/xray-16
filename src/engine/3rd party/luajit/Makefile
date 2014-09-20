# makefile for installing Lua
# see INSTALL for installation instructions
# see src/Makefile and src/luaconf.h for further customization

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

# Where to install. The installation starts in the src directory, so take care
# if INSTALL_TOP is not an absolute path. (Man pages are installed from the
# doc directory.) You may want to make these paths consistent with LUA_ROOT,
# LUA_LDIR, and LUA_CDIR in luaconf.h (and also with etc/luajit.pc).
#
INSTALL_TOP= /usr/local
INSTALL_BIN= $(INSTALL_TOP)/bin
INSTALL_INC= $(INSTALL_TOP)/include
INSTALL_LIB= $(INSTALL_TOP)/lib
INSTALL_MAN= $(INSTALL_TOP)/man/man1
INSTALL_LMOD= $(INSTALL_TOP)/share/lua/$V
INSTALL_CMOD= $(INSTALL_TOP)/lib/lua/$V

# How to install. If you don't have "install" (unlikely) then get install-sh at
#	http://dev.w3.org/cvsweb/libwww/config/install-sh
# or use cp instead.
INSTALL_EXEC= $(INSTALL) -p -m 0755
INSTALL_DATA= $(INSTALL) -p -m 0644

# Utilities.
INSTALL= install
MKDIR= mkdir

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

# Convenience platforms targets.
PLATS= linux bsd macosx solaris mingw cygwin posix generic linux_rl bsd_rl macosx_rl

# What to install.
TO_BIN= luajit
###TO_INC= lua.h luaconf.h lualib.h lauxlib.h ../etc/lua.hpp
###TO_LIB= liblua.a
###TO_MAN= lua.1 luac.1

# Lua version and release.
V= 5.1
R= 5.1.3
# LuaJIT version.
JV= 1.1.4

all:	$(PLAT)

$(PLATS) clean:
	cd src && $(MAKE) $@

test:	dummy
	src/luajit -O -e 'io.write("Hello world, from ", jit.version, "!\n")'

install: dummy
	cd src && $(MKDIR) -p $(INSTALL_BIN) $(INSTALL_INC) $(INSTALL_LIB) $(INSTALL_MAN) $(INSTALL_LMOD) $(INSTALL_CMOD) $(INSTALL_LMOD)/jit
	cd src && $(INSTALL_EXEC) $(TO_BIN) $(INSTALL_BIN)
	###cd src && $(INSTALL_DATA) $(TO_INC) $(INSTALL_INC)
	###cd src && $(INSTALL_DATA) $(TO_LIB) $(INSTALL_LIB)
	###cd doc && $(INSTALL_DATA) $(TO_MAN) $(INSTALL_MAN)
	cd jit && $(INSTALL_DATA) *.lua $(INSTALL_LMOD)/jit

none:
	@echo "Please do"
	@echo "   make PLATFORM"
	@echo "where PLATFORM is one of these:"
	@echo "   $(PLATS)"
	@echo "See jitdoc/luajit_install.html for complete instructions."

# make may get confused with test/ and INSTALL in a case-insensitive OS
dummy:

# echo config parameters
echo:
	@echo ""
	@echo "These are the parameters currently set in src/Makefile to build LuaJIT $(JV):"
	@echo ""
	@cd src && $(MAKE) -s echo
	@echo ""
	@echo "These are the parameters currently set in Makefile to install LuaJIT $(JV):"
	@echo ""
	@echo "PLAT = $(PLAT)"
	@echo "INSTALL_TOP = $(INSTALL_TOP)"
	@echo "INSTALL_BIN = $(INSTALL_BIN)"
	@echo "INSTALL_INC = $(INSTALL_INC)"
	@echo "INSTALL_LIB = $(INSTALL_LIB)"
	@echo "INSTALL_MAN = $(INSTALL_MAN)"
	@echo "INSTALL_LMOD = $(INSTALL_LMOD)"
	@echo "INSTALL_CMOD = $(INSTALL_CMOD)"
	@echo "INSTALL_EXEC = $(INSTALL_EXEC)"
	@echo "INSTALL_DATA = $(INSTALL_DATA)"
	@echo ""
	@echo "See also src/luaconf.h ."
	@echo ""

# echo private config parameters
pecho:
	@echo "V = $(V)"
	@echo "R = $(R)"
	@echo "JV = $(JV)"
	@echo "TO_BIN = $(TO_BIN)"
	@echo "TO_INC = $(TO_INC)"
	@echo "TO_LIB = $(TO_LIB)"
	@echo "TO_MAN = $(TO_MAN)"

# echo config parameters as Lua code
# uncomment the last sed expression if you want nil instead of empty strings
lecho:
	@echo "-- installation parameters for Lua $(R), LuaJIT $(JV)"
	@echo "VERSION = '$(V)'"
	@echo "RELEASE = '$(R)'"
	@echo "LUAJIT_VERSION = '$(JV)'"
	@$(MAKE) echo | grep = | sed -e 's/= /= "/' -e 's/$$/"/' #-e 's/""/nil/'
	@echo "-- EOF"

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) clean test install local none dummy echo pecho lecho

# (end of Makefile)
