I couldn't get configure.exe to work.  If you can't either, copy the config.h 

from this directory to [ode]/include/ode/config.h and try that.  It works for

me, but I make no promises.



changes were made to:

	msvcdefs.def (added a few missing functions)

	misc.cpp (added dInfinityValue)