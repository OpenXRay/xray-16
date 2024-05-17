# - Try to find libepoxy
# Once done this will define
#
#  epoxy_FOUND        - System has libepoxy
#  epoxy_LIBRARY      - The libepoxy library
#  epoxy_INCLUDE_DIR  - The libepoxy include dir
#  epoxy_DEFINITIONS  - Compiler switches required for using libepoxy

# Copyright (c) 2014 Fredrik Höglund <fredrik@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PKG_epoxy QUIET epoxy)

set(epoxy_DEFINITIONS ${PKG_epoxy_CFLAGS})

find_path(epoxy_INCLUDE_DIR NAMES epoxy/gl.h HINTS ${PKG_epoxy_INCLUDEDIR} ${PKG_epoxy_INCLUDE_DIRS})
find_library(epoxy_LIBRARY  NAMES epoxy      HINTS ${PKG_epoxy_LIBDIR} ${PKG_epoxy_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(epoxy DEFAULT_MSG epoxy_LIBRARY epoxy_INCLUDE_DIR)

mark_as_advanced(epoxy_INCLUDE_DIR epoxy_LIBRARY)
