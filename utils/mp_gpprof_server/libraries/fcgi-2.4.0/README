FastCGI Developer's Kit README
------------------------------

    $Id: README,v 1.21 2003/01/19 17:19:41 robs Exp $
    Copyright (c) 1996 Open Market, Inc.
    See the file "LICENSE.TERMS" for information on usage and redistribution
    of this file, and for a DISCLAIMER OF ALL WARRANTIES.

Basic Directions
----------------

Unix:

    ./configure
    make
    make install

Win32:

    nmake -f Makefile.nt

    (or use the MSVC++ project files in the Win32 directory)


CHANGES
-------

For more detail regarding changes, please consult the cvs log available 
on http://fastcgi.com/.


2.4.0
-----

 *) When closing connections, shutdown() the send side of TCP sockets to 
    prevent a TCP RST from trashing the reciept of data on the client (when
    the client continues to send data to the application).

 *) [WIN32] force an exit from the ShutdownRequestThread when a shutdown is
    signaled and NamedPipes are in use.

 *) Use streamsize and char_type in the C++ API.

 *) [WIN32] Eliminate the (partial and broken) use of OverlappedIO - this 
    was causing a loose spin in acceptNamedPipe().

 *) Fix a bug that caused an assert to pop when an async file descriptor was
    numbered greater than 16. Kevin Eye [eye@buffalo.edu]

 *) Update the echo-cpp example to show the restoral of the original
    streambufs.  Trub, Vladimir [vtrub@purolator.com]

 *) Fix a bug a that caused the lib to crash under certain circumstances
    when an error occured on a read

 *) Test for iostreams that support a streambuf assigment operator

 *) (WIN32) Fixed initialization of the accept mutex when OpenSocket() was used.
    Niklas Bergh [niklas.bergh@tific.com]


2.2.2  
-----

 *) Added support for shared libraries.

 *) Added support for a graceful shutdown via an event under Win32.

 *) Added default signal handlers for PIPE, USR1, and TERM.

 *) Fix some minor bugs in the 0S_ layer.

 *) Fixed the C++ streambuf implementation.


Changes with devkit 2.1.1 
-------------------------

 *) Fixed an unintentional sign extension during promotion  in Java's
    FCGIInputStream.read(). Takayuki Tachikawa <tachi@po.ntts.co.jp>

 *) Cleaned up warnings in examples (mostly main() complaints).

 *) Removed examples/tiny-cgi.c (it wasn't a FastCGI application?!).

 *) Remove some debugging code and clean up some gcc warnings in cgi-fcgi.c.

 *) Add multithread support to the fcgiapp lib and an example multithreaded
    application, threaded.c.  Based on work by Dennis Payne
    <dpayne@softscape.com> and Gene Sokolov <hook@aktrad.ru>.

 *) Remove the printf() and #include of stdio.h from examples/echo2.c.

 *) Remove the static initialization of _fcgi_sF[] because on glibc 2.x based
    systems stdin/stdout/stderr are no longer static.

 *) Flush FastCGI buffers at application exit.  <eichin@fastengines.com>

 << INSERT OTHER STUFF HERE >>


What's New: Version 2.0b2, 04 April 1997
--------------------------------------

Some additional bug fixes, mostly on NT port.  The following list
of the bugs that have been and fixed:
  1. Updated build_no_shell.bat to create a FcgiBin directory under the
     top level of the FastCGI kit and copy all executables and the
     FastCGI dll there.  This makes it easier to use.
  2. Corrected the Unix version of OS_SpawnChild so that it didn't close
     the listenFd when forking off child processes.  This code would
     affect the cgi-fcgi application on Unix.  The problem is that it
     could only start one fastcgi process.  Any other processes would not
     get the listen file descriptor and they would die.
  3. Corrected cgi-fcgi.c so that it properly handled large posts.  The
     bug was introduced with the asynchronous I/O model implemented for
     the Windows NT port.  The problem was not clearing a bit indicating
     that a read had completed.  This caused the application to stall.
  4. Corrected OS_DoIo, the function used for scheduling I/O for cgi-fcgi.
     It had a bug where it wasn't creating a copy of the file descriptors
     used for I/O.  This would cause the master list of FDs to watch to be
     reset and thus would hang the application because we would no longer
     watch for I/O on those file descriptors. (This problem was specific to
     Unix and only happened with the cgi-fcgi application.)
  5. Cleaned up several compilation warnings present on OSF.


What's New: Version 2.0b1, 24 March 1997
--------------------------------------

This "beta" release adds the functionality of "cgi-fcgi" to the
Windows NT platform and allows for creation of FastCGI applications
running in Win32 environment.  There is almost no new documentation
provided, but will become part of this kit in the official release.
  1. Added FastCGI libraries running on Windows NT 3.51+
  2. Rename errno to FCGI_errno in the FCGX_Stream, which was causing
     problems on some Linux platforms and NT.
  3. Fixed a parenthesis problem in FCGI_gets


What's New: Version 1.5.1, 12 December 1996
--------------------------------------

This release introduces mostly bug fixes, without any additional
functionality to the kit.
  1. Conditional compilation for the hp-ux compiler.
  2. Loop around the accept() call to eliminate "OS Error: Interrupted
     System Call" message from appearing in the error logs.
  3. Casting of the FCGI_Header to (char *), which eliminates the
     assertion failure "bufPtr->size>0".


What's New: Version 1.5, 12 June 1996
--------------------------------------

General:

  Added a white paper on FastCGI application performance to the
  doc directory.  Generally brought the other docs up to date.

  Rearranged the kit to put more emphasis on running FastCGI-capable
  servers and less on running cgi-fcgi.  Added
  examples/conf/om-httpd.config, a config file that demonstrates all
  of the example apps.  (Would like to have similar configs for NCSA
  and Apache.)

  Added the tiny-authorizer and sample-store applications to
  the examples.  These are explained in the index.html.

    In addition to everything else it does, sample-store demonstrates
    a bug in the Open Market WebServer 2.0: When an Authorizer
    application denies access, the server tacks some extra junk onto
    the end of the page the application returns.  A little ugly but
    not fatal.

C libraries:

  Added the functions FCGX_Finish and FCGI_Finish.  These functions
  finish the current request from the HTTP server but do not begin a
  new request.  These functions make it possible for applications to
  perform other processing between requests.  An application must not
  use its stdin, stdout, stderr, or environ between calling
  FCGI_Finish and calling FCGI_Accept.  See doc/FCGI_Finish.3 for
  more information.  The application examples/sample-store.c demonstrates
  the use of FCGI_Finish.

  Added conditional 'extern "C"' stuff to the .h files fcgi_stdio.h,
  fcgiapp.h, and fcgiappmisc.h for the benefit of C++ applications
  (suggested by Jim McCarthy).

  Fixed two bugs in FCGX_VFPrintF (reported by Ben Laurie).  These
  bugs affected processing of %f format specifiers and of all format
  specifiers containing a precision spec (e.g "%12.4g").

  Fixed a bug in FCGX_Accept in which the environment variable
  FCGI_WEBSERVER_ADDRS was being read rather than the specified
  FCGI_WEB_SERVER_ADDRS.  Fixed a bug in FCGX_Accept in which the
  wrong storage was freed when FCGI_WEB_SERVER_ADDRS contained more
  than one address or if the address check failed.

  Changed FCGX_Accept to avoid depending upon accept(2) returning the
  correct value of sin_family in the socketaddr structure for an
  AF_UNIX connection (SCO returns the wrong value, as reported by Paul
  Mahoney).

  Changed the error retry logic in FCGX_Accept.  FCGX_Accept now
  returns -1 only in case of operating system errors that occur while
  accepting a connection (e.g. out of file descriptors).  Other errors
  cause the current connection to be dropped and a new connection to
  be attempted.

Perl:

  Changed FCGI.xs to make it insensitive to Perl's treatment of
  environ (we hope).  Changed FCGI::accept so the initial environment
  variables are not unset on the first call to FCGI::accept (or on
  subsequent calls either).  Added the echo-perl example
  program.  Added a workaround for the "empty initial environment bug"
  to tiny-perl-fcgi.  Changed the example Perl scripts to use a new
  symbolic link ./perl, avoiding the HP-UX 32 character limit on the
  first line of a command interpreter file.

  Because the FastCGI-enabled Perl interpreter uses the C fcgi_stdio
  library, it picks up all the changes listed above for C.  There's
  a new Perl subroutine FCGI::finish.

Tcl:

  Fixed a bug in tclFCGI.c that caused the request environment
  variables to be lost.  Changed FCGI_Accept so the initial
  environment variables are not unset on the first call to FCGI_Accept
  (or on subsequent calls either).  Added the echo-tcl example
  program.  Fixed another bug that caused Tcl to become confused by
  file opens; as a side effect of this change, writes to stdout/stderr
  that occur in an app running as FastCGI before FCGI_Accept is called
  are no-ops rather than crashing Tcl.  Changed the example Tcl
  scripts to use a new symbolic link ./tclsh, avoiding the HP-UX 32
  character limit on the first line of a command interpreter file.

  Because the FastCGI-enabled Tcl interpreter uses the C fcgi_stdio
  library, it picks up all the changes listed above for C; there's
  a new Tcl command FCGI_Finish.

Java:

  Fixed a sign-extension bug in FCGIMessage.java that caused bad encodings
  of names and values in name-value pairs for lengths in [128..255].
  Made small cleanups in the Java example programs to make them more
  consistent with the other examples.



What's New: Version 1.4, 10 May 1996
--------------------------------------

Includes Java classes and Java examples.



What's New: Version 1.3.1, 6 May 1996
--------------------------------------

New, simplified, license terms.  Includes an expanded whitepaper that
describes FastCGI support in Open Market's Secure WebServer 2.0.
Includes Open Market FastCGI 1.0 Programmer's Guide.  Includes
"FastCGI: A High-Performance Gateway Interface", a position paper
presented at the workshop "Programming the Web - a search for APIs",
Fifth International World Wide Web Conference, 6 May 1996, Paris,
France.



What's New: Version 1.3, 29 April 1996
--------------------------------------

First public release; new license terms on all files.

Changed cgi-fcgi.c to use SO_REUSEADDR when creating the listening socket;
this avoids the need to wait through the TIME_WAIT state on all the TCP
connections made by the previous instance of an external application
you are restarting.



What's New: Version 1.2.2, 15 April 1996
----------------------------------------

Partially fixed a bug in Perl's FCGI::accept (source file FCGI.xs).
The per-request environment variables were being lost.  Now the
per-request environment variables show up correctly, except that if
the Perl application has an empty initial environment, the environment
variables associated with the *first* request are lost.  Therefore,
when starting Perl, always set some environment variable using the
AppClass -initial-env option, or by running cgi-fcgi in a non-empty
environment.



What's New: Version 1.2.1, 22 March 1996
----------------------------------------

Fixed a bug in FCGI_Accept.  If your application running as FastCGI
opened a file before calling FCGI_Accept, it would decide that it
was really running as CGI.  Things went downhill quickly after that!

Also added advisory locking to serialize calls to accept on shared
listening sockets on Solaris and IRIX, to work around problems
with concurrent accept calls on these platforms.



What's New: Version 1.2, 20 March 1996
--------------------------------------

1. This version of the kit implements the most recent draft
of the protocol spec.  Enhancements to the protocol include
a BEGIN_REQUEST record that simplifies request ID management
and transmits role and keep-alive information, and a simplified
end-of-stream indication.

The protocol spec has been revised to describe exactly what's
been implemented, leaving out the features that we hope to
introduce in later releases.

At the application level, the visible change is the FCGI_ROLE
variable that's available to applications.  This allows an application
to check that it has been invoked in the expected role.  A single
application can be written to respond in several roles.  The
FCGI_Accept.3 manpage contains more information.

2.  We introduced the new "module" prefix FCGX in order to simplify
the relationship between fcgi_stdio and fcgiapp.

A growing number of functions are provided in both fcgi_stdio and
fcgiapp versions.  Rather than inventing an ad hoc solution for each
naming conflict (as we did with FCGI_accept and FCGI_Accept), we've
bitten the bullet and systematically renamed *all* the fcgapp
primitives with the prefix FCGX_.  In fcgi_stdio, we've renamed
FCGI_accept to FCGI_Accept.  So all functions that are common in the
two libraries have the same name modulo the different prefixes.

The Accept function visible in Tcl is now called FCGI_Accept, not
FCGI_accept.

The Accept function visible in Perl is now FCGI::accept.  All
lower case names for functions and all upper case names for
modules appears to be a Perl convention, so we conform.

3. The kit now fully supports the Responder, Authorizer,
and Filter roles.

The Filter role required a new function, FCGI_StartFilterData.
FCGI_StartFilterData changes the input stream from reading
FCGI_STDIN data to reading FCGI_DATA data.  The manpage
gives full details.

Another new function, FCGI_SetExitStatus, is primarily for
the Responder role but is available to all.  FCGI_SetExitStatus
allows an application to set a nonzero "exit" status
before completing a request and calling FCGI_Accept again.
The manpage gives full details.

These two new functions are provided at both the fcgi_stdio interface
and the basic fcgiapp interface.  Naturally, the fcgiapp versions are
called FCGX_StartFilterData and FCGX_SetExitStatus.

4. The fcgiapp interface changed slightly in order to treat
the streams and environment data more symmetrically.

FCGX_Accept now returns an environment pointer, rather than requiring
a call to FCGX_GetAllParams to retrieve an environment pointer.
FCGX_GetParam takes an explicit environment pointer argument.
FCGX_GetAllParams is eliminated.  See the documentation in the header
file for complete information.

fcgiapp also added the procedure FCGX_IsCGI, providing a standardized
test of whether the app was started as CGI or FastCGI.

5. We've ported the kits to vendor-supported ANSI C compilers
on Sun (Solaris 2.X), HP, and Digital platforms.  GCC can be
selected on these platforms by performing SETENV CC gcc before
running configure.



What's New: Version 1.1, 30 Jan 1996
------------------------------------

1. More platforms: Digital UNIX, IBM AIX, Silicon Graphics IRIX,
Sun SunOS 4.1.4.

2. Perl and Tcl: Simple recipes for producing Perl and Tcl
interpreters that run as FastCGI applications.  No source
code changes are needed to Perl and Tcl.  Documented
in separate documents, accessible via the index page.



Version 1.0, 10 Jan 1996
------------------------
