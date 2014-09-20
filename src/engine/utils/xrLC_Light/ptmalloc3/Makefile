# Makefile for ptmalloc, version 3
# by Wolfram Gloger 1996-1999, 2001-2005, 2006

DIST_FILES0 = ChangeLog malloc-2.8.3.h malloc.c \
 malloc-private.h ptmalloc3.c \
 sysdeps \
 #tst-mallocstate.c tst-mstats.c
DIST_FILES1 = COPYRIGHT README Makefile \
 $(DIST_FILES0) \
 lran2.h t-test.h t-test1.c t-test2.c \
 tst-independent-alloc.c \
 #debian
DIST_FILES2 = $(DIST_FILES1) \
 m-test1.c \
 RCS/*,v

TAR_FLAGS = --numeric-owner --exclude "*~" --exclude "debian/tmp*"

#CC = /pkg/gcc-2.95.2-wg/bin/gcc
CC = gcc

SYS_FLAGS  =
OPT_FLAGS  = -g -O2 #-O # -O2
WARN_FLAGS = -Wall -Wstrict-prototypes
SH_FLAGS   = -shared -fpic

INC_FLAGS  = -Isysdeps/generic

# Flags for the test programs
T_FLAGS   = -DUSE_MALLOC=1 -DTEST=1

# Flags for the compilation of malloc.c
M_FLAGS   = -DTHREAD_STATS=1 #-DMALLOC_DEBUG=1 -DDEBUG=1

# Thread flags.
# See the platform-specific targets below.
THR_FLAGS = -DUSE_TSD_DATA_HACK -D_REENTRANT
THR_LIBS  = -lpthread

RM        = rm -f
AR        = ar
RANLIB    = ranlib

MALLOC_OBJ = ptmalloc3.o malloc.o
LIB_MALLOC = libptmalloc3.a

T_SUF =
TESTS = t-test1$(T_SUF) t-test2$(T_SUF) \
	tst-independent-alloc$(T_SUF)
        #m-test1$(T_SUF) tst-mallocstate$(T_SUF) tst-mstats$(T_SUF)

CFLAGS = $(SYS_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(THR_FLAGS) $(INC_FLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(LIB_MALLOC) $(TESTS)

ptmalloc3.o: ptmalloc3.c malloc-2.8.3.h
	$(CC) -c $(CFLAGS) $(M_FLAGS) -DMSPACES=1 $<

malloc.o: malloc.c
	$(CC) -c $(CFLAGS) $(M_FLAGS) -DONLY_MSPACES -DUSE_LOCKS=0 $<

#malloc-stats.o: malloc-stats.c malloc.h
#	$(CC) -c $(CFLAGS) $(M_FLAGS) $<

libptmalloc3.a: $(MALLOC_OBJ)
	$(AR) cr $@ $(MALLOC_OBJ)
	$(RANLIB) $@

libptmalloc3.so: $(MALLOC_OBJ)
	$(CC) $(SH_FLAGS) $(CFLAGS) $(M_FLAGS) $(MALLOC_OBJ) -o $@

again:
	$(RM) $(TESTS)
	$(MAKE) $(TESTS)

clean:
	$(RM) $(MALLOC_OBJ) libptmalloc3.a libptmalloc3.so $(TESTS) \
         core core.[0-9]*

m-test1$(T_SUF): m-test1.c $(LIB_MALLOC)
	$(CC) $(CFLAGS) $(T_FLAGS) m-test1.c $(LIB_MALLOC) $(THR_LIBS) -o $@

t-test1$(T_SUF): t-test1.c t-test.h $(LIB_MALLOC)
	$(CC) $(CFLAGS) $(T_FLAGS) t-test1.c $(LIB_MALLOC) $(THR_LIBS) -o $@

t-test2$(T_SUF): t-test2.c t-test.h $(LIB_MALLOC)
	$(CC) $(CFLAGS) $(T_FLAGS) t-test2.c $(LIB_MALLOC) $(THR_LIBS) -o $@

tst-mallocstate$(T_SUF): tst-mallocstate.c $(LIB_MALLOC)
	$(CC) $(CFLAGS) $(T_FLAGS) tst-mallocstate.c $(LIB_MALLOC) \
	 $(THR_LIBS) -o $@

tst-mstats$(T_SUF): tst-mstats.c $(LIB_MALLOC)
	$(CC) $(CFLAGS) $(T_FLAGS) tst-mstats.c $(LIB_MALLOC) \
	 $(THR_LIBS) -o $@

tst-independent-alloc$(T_SUF): tst-independent-alloc.c $(LIB_MALLOC)
	$(CC) $(CFLAGS) $(T_FLAGS) tst-independent-alloc.c $(LIB_MALLOC) \
	 $(THR_LIBS) -o $@

############################################################################
# Platform-specific targets. The ones ending in `-libc' are provided
# to enable comparison with the standard malloc implementation from
# the system's native C library.  The option USE_TSD_DATA_HACK is now
# the default for pthreads systems, as most (Irix 6, Solaris 2) seem
# to need it.  Try with USE_TSD_DATA_HACK undefined only if you're
# confident that your systems's thread specific data functions do _not_
# use malloc themselves.

# posix threads with TSD data hack
posix:
	$(MAKE) THR_FLAGS='-DUSE_TSD_DATA_HACK -D_REENTRANT' \
 OPT_FLAGS='$(OPT_FLAGS)' SYS_FLAGS='$(SYS_FLAGS)' CC='$(CC)' \
 INC_FLAGS='-Isysdeps/pthread -Isysdeps/generic -I.'
 THR_LIBS=-lpthread

# posix threads with explicit initialization.  Known to be needed on HPUX.
posix-explicit:
	$(MAKE) THR_FLAGS='-D_REENTRANT -DUSE_TSD_DATA_HACK -DUSE_STARTER=2' \
	 THR_LIBS=-lpthread \
	 OPT_FLAGS='$(OPT_FLAGS)' SYS_FLAGS='$(SYS_FLAGS)' CC='$(CC)' \
	 INC_FLAGS='-Isysdeps/pthread -Isysdeps/generic -I.' \
	 M_FLAGS='$(M_FLAGS)'

# posix threads without TSD data hack -- not known to work
posix-with-tsd:
	$(MAKE) THR_FLAGS='-D_REENTRANT' THR_LIBS=-lpthread \
	INC_FLAGS='-Isysdeps/pthread -Isysdeps/generic -I.' \
	M_FLAGS='$(M_FLAGS)'

posix-libc:
	$(MAKE) THR_FLAGS='-D_REENTRANT' THR_LIBS=-lpthread \
	INC_FLAGS='-Isysdeps/pthread -Isysdeps/generic -I.' \
	M_FLAGS='$(M_FLAGS)' LIB_MALLOC= T_FLAGS= T_SUF=-libc

linux-pthread:
	$(MAKE) SYS_FLAGS='-D_GNU_SOURCE=1' \
 WARN_FLAGS='-Wall -Wstrict-prototypes' \
 OPT_FLAGS='$(OPT_FLAGS)' THR_FLAGS='-DUSE_TSD_DATA_HACK' \
 INC_FLAGS='-Isysdeps/pthread -Isysdeps/generic -I.' M_FLAGS='$(M_FLAGS)' \
 TESTS='$(TESTS)'

linux-shared:
	$(MAKE) SYS_FLAGS='-D_GNU_SOURCE=1 -fpic' \
 WARN_FLAGS='-Wall -Wstrict-prototypes' \
 OPT_FLAGS='$(OPT_FLAGS)' THR_FLAGS='-DUSE_TSD_DATA_HACK' \
 INC_FLAGS='-Isysdeps/pthread -Isysdeps/generic -I.' M_FLAGS='$(M_FLAGS)' \
 LIB_MALLOC=libptmalloc3.so

sproc:
	$(MAKE) THR_FLAGS='' THR_LIBS='' OPT_FLAGS='$(OPT_FLAGS)' CC='$(CC)' \
	INC_FLAGS='-Isysdeps/sproc -Isysdeps/generic -I.' \
	M_FLAGS='$(M_FLAGS) -Dmalloc_getpagesize=4096'

sproc-shared:
	$(MAKE) THR_FLAGS='' THR_LIBS= \
	 SH_FLAGS='-shared -check_registry /usr/lib/so_locations' \
	 INC_FLAGS='-Isysdeps/sproc -Isysdeps/generic -I.' \
	  LIB_MALLOC=libptmalloc3.so M_FLAGS='$(M_FLAGS)'

sproc-libc:
	$(MAKE) THR_FLAGS='' THR_LIBS= LIB_MALLOC= T_FLAGS= \
	 INC_FLAGS='-Isysdeps/sproc -Isysdeps/generic -I.' \
	 T_SUF=-libc M_FLAGS='$(M_FLAGS)'

solaris:
	$(MAKE) THR_FLAGS='-D_REENTRANT' OPT_FLAGS='$(OPT_FLAGS)' \
	 INC_FLAGS='-Isysdeps/solaris -Isysdeps/generic -I.' \
	THR_LIBS=-lthread M_FLAGS='$(M_FLAGS)'

solaris-libc:
	$(MAKE) THR_FLAGS='-D_REENTRANT' OPT_FLAGS='$(OPT_FLAGS)' \
	 INC_FLAGS='-Isysdeps/solaris -Isysdeps/generic -I.' \
 THR_LIBS=-lthread LIB_MALLOC= T_FLAGS= T_SUF=-libc M_FLAGS='$(M_FLAGS)'

nothreads:
	$(MAKE) OPT_FLAGS='$(OPT_FLAGS)' SYS_FLAGS='$(SYS_FLAGS)' \
	 INC_FLAGS='-Isysdeps/generic -I.' \
 THR_FLAGS='' THR_LIBS='' M_FLAGS='$(M_FLAGS)'

gcc-nothreads:
	$(MAKE) CC='gcc' WARN_FLAGS='-Wall' OPT_FLAGS='$(OPT_FLAGS)' \
	 INC_FLAGS='-Isysdeps/generic -I.' \
 SYS_FLAGS='$(SYS_FLAGS)' THR_FLAGS='' THR_LIBS='' M_FLAGS='$(M_FLAGS)'

linux-nothreads:
	$(MAKE) CC='gcc' WARN_FLAGS='-Wall' OPT_FLAGS='$(OPT_FLAGS)' \
	 INC_FLAGS='-Isysdeps/generic -I.' \
 SYS_FLAGS='-D_GNU_SOURCE' THR_FLAGS='' THR_LIBS='' M_FLAGS='$(M_FLAGS)'

############################################################################

check: $(TESTS)
	./t-test1
	./t-test2
	#./tst-mallocstate || echo "Test mallocstate failed!"
	#./tst-mstats || echo "Test mstats failed!"

snap:
	cd ..; tar $(TAR_FLAGS) -c -f - $(DIST_FILES1:%=ptmalloc3/%) | \
	 gzip -9 >ptmalloc3-current.tar.gz

dist:
	cd ..; tar $(TAR_FLAGS) -c -f - $(DIST_FILES2:%=ptmalloc3/%) | \
	 gzip -9 >ptmalloc3.tar.gz

# dependencies
ptmalloc3.o: malloc-private.h
