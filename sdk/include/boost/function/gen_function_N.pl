#!/usr/bin/perl -w
#
# Boost.Function library
#
# Copyright (C) 2001-2003 Doug Gregor (gregod@cs.rpi.edu)
#
# Permission to copy, use, sell and distribute this software is granted
# provided this copyright notice appears in all copies.
# Permission to modify the code and to distribute modified code is granted
# provided this copyright notice appears in all copies, and a notice
# that the code was modified is included with the copyright notice.
#
# This software is provided "as is" without express or implied warranty,
# and with no claim as to its suitability for any purpose.
#
# For more information, see http://www.boost.org
use English;

if ($#ARGV < 0) {
  print "Usage: perl gen_function_N <number of arguments>\n";
  exit;
}


$totalNumArgs = $ARGV[0];
for ($numArgs = 0; $numArgs <= $totalNumArgs; ++$numArgs) {
  open OUT, ">function$numArgs.hpp";
  print OUT "#define BOOST_FUNCTION_NUM_ARGS $numArgs\n";
  print OUT "#include <boost/function/detail/maybe_include.hpp>\n";
  print OUT "#undef BOOST_FUNCTION_NUM_ARGS\n";
  close OUT;
}
